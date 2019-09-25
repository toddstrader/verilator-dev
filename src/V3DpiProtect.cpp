// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Build DPI protected C++ and SV
//
// Code available from: http://www.veripool.org/verilator
//
//*************************************************************************
//
// Copyright 2003-2019 by Todd Strader.  This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
//
// Verilator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//*************************************************************************

#include "config_build.h"
#include "verilatedos.h"

#include "V3Global.h"
#include "V3String.h"
#include "V3DpiProtect.h"
#include "V3File.h"

#include <list>


//######################################################################
// DpiProtect top-level visitor

class ProtectVisitor : public AstNVisitor {
  private:
    bool m_modProtected;                // Tracks if we've already protected on module
    AstVFile* m_vfilep;                 // DPI-enabled Verilog wrapper
    AstCFile* m_cfilep;                 // C implementation of DPI functions
    // Verilog text blocks
    AstTextBlock* m_modPortsp;          // Module port list
    AstTextBlock* m_comboPortsp;        // Combo function port list
    AstTextBlock* m_seqPortsp;          // Sequential function port list
    AstTextBlock* m_comboIgnorePortsp;  // Combo ignore function port list
    AstTextBlock* m_comboDeclsp;        // Combo signal declaration list
    AstTextBlock* m_seqDeclsp;          // Sequential signal declaration list
    AstTextBlock* m_tmpDeclsp;          // Temporary signal declaration list
    AstTextBlock* m_comboParamsp;       // Combo function parameter list
    AstTextBlock* m_clkSensp;           // Clock sensitivity list
    AstTextBlock* m_comboIgnoreParamsp; // Combo ignore parameter list
    AstTextBlock* m_seqParamsp;         // Sequential parameter list
    AstTextBlock* m_nbAssignsp;         // Non-blocking assignment list
    AstTextBlock* m_seqAssignsp;        // Sequential assignment list
    AstTextBlock* m_comboAssignsp;      // Combo assignment list
    // C text blocks
    AstTextBlock* m_cComboParamsp;      // Combo function parameter list
    AstTextBlock* m_cComboInsp;         // Combo input copy list
    AstTextBlock* m_cComboOutsp;        // Combo output copy list
    AstTextBlock* m_cSeqParamsp;        // Sequential parameter list
    AstTextBlock* m_cSeqClksp;          // Sequential clock copy list
    AstTextBlock* m_cSeqOutsp;          // Sequential output copy list
    AstTextBlock* m_cIgnoreParamsp;     // Combo ignore parameter list
    string m_libName;
    string m_topName;

    // VISITORS
    virtual void visit(AstNetlist* nodep) {
        m_vfilep = new AstVFile(nodep->fileline(),
                                v3Global.opt.makeDir()+"/"+m_libName+".sv");
        nodep->addFilesp(m_vfilep);
        m_cfilep = new AstCFile(nodep->fileline(),
                                v3Global.opt.makeDir()+"/"+m_libName+".cpp");
        nodep->addFilesp(m_cfilep);
        iterateChildren(nodep);
    }

    virtual void visit(AstNodeModule* nodep) {
        if (m_modProtected) {
            v3Global.rootp()->v3error("Unsupported: --dpi-protect with multiple"
                                      " top-level modules");
        }
        m_modProtected = true;
        createSvFile(nodep->fileline());
        createCppFile(nodep->fileline());

        iterateChildren(nodep);
    }

    void addComment(AstTextBlock* txtp, FileLine* fl, const string& comment) {
        txtp->addNodep(new AstComment(fl, comment));
    }

    void initialComment(AstTextBlock* txtp, FileLine* fl) {
        addComment(txtp, fl, "Creates an instance of the secret module at initial-time");
        addComment(txtp, fl, "(one for each instance in the user's design) also evaluates");
        addComment(txtp, fl, "the secret module's initial process");
    }

    void comboComment(AstTextBlock* txtp, FileLine* fl) {
        addComment(txtp, fl, "Updates all non-clock inputs and retrieves the results");
    }

    void seqComment(AstTextBlock* txtp, FileLine* fl) {
        addComment(txtp, fl, "Updates all clocks and retrieves the results");
    }

    void comboIgnoreComment(AstTextBlock* txtp, FileLine* fl) {
        addComment(txtp, fl, "Need to convince some simulators that the input to the module");
        addComment(txtp, fl, "must be evaluated before evaluating the clock edge");
    }

    void finalComment(AstTextBlock* txtp, FileLine* fl) {
        addComment(txtp, fl, "Evaluates the secret module's final process");
    }

    void createSvFile(FileLine* fl) {
        // Comments
        AstTextBlock* txtp = new AstTextBlock(fl);
        addComment(txtp, fl, "Wrapper module for DPI protected library");
        addComment(txtp, fl, "This module requires lib"+m_libName+
                   ".a or lib"+m_libName+".so to work");
        addComment(txtp, fl, "See instructions in your simulator for how"
                   " to use DPI libraries\n");

        // Module declaration
        m_modPortsp = new AstTextBlock(fl, "module "+m_libName+" (\n", false, true);
        txtp->addNodep(m_modPortsp);
        txtp->addText(fl, ");\n\n");

        // DPI declarations
        initialComment(txtp, fl);
        txtp->addText(fl, "import \"DPI-C\" function chandle "+
                      m_libName+"_dpiprotect_create (string scope__V);\n\n");
        comboComment(txtp, fl);
        m_comboPortsp = new AstTextBlock(fl, "import \"DPI-C\" function void "+
                                         m_libName+"_dpiprotect_combo_update "
                                         "(\n", false, true);
        m_comboPortsp->addText(fl, "chandle handle__V\n");
        txtp->addNodep(m_comboPortsp);
        txtp->addText(fl, ");\n\n");
        seqComment(txtp, fl);
        m_seqPortsp = new AstTextBlock(fl, "import \"DPI-C\" function void "+
                                       m_libName+"_dpiprotect_seq_update "
                                       "(\n", false, true);
        m_seqPortsp->addText(fl, "chandle handle__V\n");
        txtp->addNodep(m_seqPortsp);
        txtp->addText(fl, ");\n\n");
        comboIgnoreComment(txtp, fl);
        m_comboIgnorePortsp = new AstTextBlock(fl, "import \"DPI-C\" function void "+
                                               m_libName+"_dpiprotect_combo_ignore "
                                               "(\n", false, true);
        m_comboIgnorePortsp->addText(fl, "chandle handle__V\n");
        txtp->addNodep(m_comboIgnorePortsp);
        txtp->addText(fl, ");\n\n");
        finalComment(txtp, fl);
        txtp->addText(fl, "import \"DPI-C\" function void "+
                      m_libName+"_dpiprotect_final (chandle handle__V);\n\n");

        // Local variables
        txtp->addText(fl, "chandle handle__V;\n\n");
        m_comboDeclsp = new AstTextBlock(fl);
        txtp->addNodep(m_comboDeclsp);
        m_seqDeclsp = new AstTextBlock(fl);
        txtp->addNodep(m_seqDeclsp);
        m_tmpDeclsp = new AstTextBlock(fl);
        txtp->addNodep(m_tmpDeclsp);
        txtp->addText(fl, "\ntime last_combo_time__V;\n");
        txtp->addText(fl, "time last_seq_time__V;\n\n");

        // Initial
        txtp->addText(fl, "initial handle__V = "+m_libName+"_dpiprotect_create"
                      "($sformatf(\"%m\"));\n\n");

        // Combinatorial process
        addComment(txtp, fl, "Combinatorialy evaluate changes to inputs");
        m_comboParamsp = new AstTextBlock(fl, "always @(*) begin\n"+
                                          m_libName+"_dpiprotect_combo_update(\n",
                                          false, true);
        m_comboParamsp->addText(fl, "handle__V\n");
        txtp->addNodep(m_comboParamsp);
        txtp->addText(fl, ");\n");
        txtp->addText(fl, "last_combo_time__V = $time;\n");
        txtp->addText(fl, "end\n\n");

        // Sequential process
        addComment(txtp, fl, "Evaluate clock edges");
        m_clkSensp = new AstTextBlock(fl, "always @(", false, true);
        txtp->addNodep(m_clkSensp);
        txtp->addText(fl, ") begin\n");
        m_comboIgnoreParamsp = new AstTextBlock(fl, m_libName+"_dpiprotect_combo_ignore(\n",
                                                false, true);
        m_comboIgnoreParamsp->addText(fl, "handle__V\n");
        txtp->addNodep(m_comboIgnoreParamsp);
        txtp->addText(fl, ");\n");
        m_seqParamsp = new AstTextBlock(fl, m_libName+"_dpiprotect_seq_update(\n",
                                        false, true);
        m_seqParamsp->addText(fl, "handle__V\n");
        txtp->addNodep(m_seqParamsp);
        txtp->addText(fl, ");\n");
        m_nbAssignsp = new AstTextBlock(fl);
        m_nbAssignsp->addText(fl, "last_seq_time__V <= $time;\n");
        txtp->addNodep(m_nbAssignsp);
        txtp->addText(fl, "end\n\n");

        // Select between combinatorial and sequential results
        addComment(txtp, fl, "Select between combinatorial and sequential results");
        txtp->addText(fl, "always @(*) begin\n");
        m_seqAssignsp = new AstTextBlock(fl, "if (last_seq_time__V > "
                                         "last_combo_time__V) begin\n");
        txtp->addNodep(m_seqAssignsp);
        m_comboAssignsp = new AstTextBlock(fl, "end else begin\n");
        txtp->addNodep(m_comboAssignsp);
        txtp->addText(fl, "end\n");
        txtp->addText(fl, "end\n\n");

        // Final
        txtp->addText(fl, "final "+m_libName+"_dpiprotect_final(handle__V);\n\n");

        txtp->addText(fl, "endmodule\n");
        m_vfilep->tblockp(txtp);
    }

    void castPtr(FileLine* fl, AstTextBlock* txtp) {
        txtp->addText(fl, m_topName+"* handlep__V = static_cast<"+m_topName+"*>(vhandlep__V);\n");
    }

    void createCppFile(FileLine* fl) {
        // Comments
        AstTextBlock* txtp = new AstTextBlock(fl);
        addComment(txtp, fl, "Wrapper functions for DPI protected library\n");

        // Includes
        txtp->addText(fl, "#include \""+m_topName+".h\"\n");
        txtp->addText(fl, "#include \"svdpi.h\"\n\n");

        // Extern C
        txtp->addText(fl, "extern \"C\" {\n\n");

        // Initial
        initialComment(txtp, fl);
        txtp->addText(fl, "void* "+m_libName+"_dpiprotect_create"
                      " (const char* scopep__V) {\n");
        txtp->addText(fl, "assert(sizeof(WData) == sizeof(svBitVecVal));\n");
        txtp->addText(fl, m_topName+"* handlep__V = new "+m_topName+"(scopep__V);\n");
        txtp->addText(fl, "return handlep__V;\n");
        txtp->addText(fl, "}\n\n");

        // Updates
        comboComment(txtp, fl);
        m_cComboParamsp = new AstTextBlock(fl, "void "+m_libName+"_dpiprotect_combo_update (\n",
                                           false, true);
        m_cComboParamsp->addText(fl, "void* vhandlep__V\n");
        txtp->addNodep(m_cComboParamsp);
        txtp->addText(fl, ")\n");
        m_cComboInsp = new AstTextBlock(fl, "{\n");
        castPtr(fl, m_cComboInsp);
        txtp->addNodep(m_cComboInsp);
        m_cComboOutsp = new AstTextBlock(fl, "handlep__V->eval();\n");
        txtp->addNodep(m_cComboOutsp);
        txtp->addText(fl, "}\n\n");

        seqComment(txtp, fl);
        m_cSeqParamsp = new AstTextBlock(fl, "void "+m_libName+"_dpiprotect_seq_update (\n",
                                         false, true);
        m_cSeqParamsp->addText(fl, "void* vhandlep__V\n");
        txtp->addNodep(m_cSeqParamsp);
        txtp->addText(fl, ")\n");
        m_cSeqClksp = new AstTextBlock(fl, "{\n");
        castPtr(fl, m_cSeqClksp);
        txtp->addNodep(m_cSeqClksp);
        m_cSeqOutsp = new AstTextBlock(fl, "handlep__V->eval();\n");
        txtp->addNodep(m_cSeqOutsp);
        txtp->addText(fl, "}\n\n");

        comboIgnoreComment(txtp, fl);
        m_cIgnoreParamsp = new AstTextBlock(fl, "void "+m_libName+"_dpiprotect_combo_ignore (\n",
                                            false, true);
        m_cIgnoreParamsp->addText(fl, "void* vhandlep__V\n");
        txtp->addNodep(m_cIgnoreParamsp);
        txtp->addText(fl, ")\n");
        txtp->addText(fl, "{ }\n\n");

        // Final
        finalComment(txtp, fl);
        txtp->addText(fl, "void "+m_libName+"_dpiprotect_final (void* vhandlep__V) {\n");
        castPtr(fl, txtp);
        txtp->addText(fl, "handlep__V->final();\n");
        txtp->addText(fl, "delete handlep__V;\n");
        txtp->addText(fl, "}\n\n");

        txtp->addText(fl, "}\n");
        m_cfilep->tblockp(txtp);
    }

    virtual void visit(AstVar* nodep) {
        if (!nodep->isIO()) return;
        if (VN_IS(nodep->dtypep(), UnpackArrayDType)) {
            nodep->v3fatalSrc("Unsupported: unpacked arrays with --dpi-protect");
        }
        if (nodep->direction() == VDirection::INPUT) {
            if (nodep->isUsedClock()) {
                handleClock(nodep);
            } else {
                handleDataInput(nodep);
            }
        } else if (nodep->direction() == VDirection::OUTPUT) {
            handleOutput(nodep);
        } else {
            nodep->v3fatalSrc("Unsupported port direction for --dpi-protect: "<<
                              nodep->direction().ascii());
        }
    }

    virtual void visit(AstNode* nodep) { }

    string sizedSvName(AstVar* varp) {
        string result;
        int width = varp->width();
        if (width > 1) result += "["+std::to_string(width-1)+":0] ";
        result += varp->name();
        return result;
    }

    string typedCppInName(AstVar* varp) {
        string result;
        int width = varp->width();
        if (width > 1) {
            result += "const svBitVecVal* ";
        } else {
            result += "unsigned char ";
        }
        result += varp->name();
        return result;
    }

    string typedCppOutName(AstVar* varp) {
        string result;
        int width = varp->width();
        if (width > 1) {
            result += "svBitVecVal* ";
        } else {
            result += "unsigned char* ";
        }
        result += varp->name();
        return result;
    }

    string cInputConnection(AstVar* varp) {
        int width = varp->width();
        int bytes = (width + 7) / 8;
        if (width == 1) {
            return "handlep__V->"+varp->name()+" = "+varp->name()+";\n";
        } else if (bytes <= sizeof(uint32_t)) {
            return "handlep__V->"+varp->name()+" = *"+varp->name()+";\n";
        } else if (bytes <= sizeof(uint64_t)) {
            return "memcpy(&(handlep__V->"+varp->name()+"), "+varp->name()+", "+std::to_string(bytes)+");\n";
        } else {
            return "memcpy(handlep__V->"+varp->name()+", "+varp->name()+", "+std::to_string(bytes)+");\n";
        }
    }

    string cOutputConnection(AstVar* varp) {
        int bytes = (varp->width() + 7) / 8;
        if (bytes <= sizeof(uint32_t)) {
            return "*"+varp->name()+" = handlep__V->"+varp->name()+";\n";
        } else if (bytes <= sizeof(uint64_t)) {
            return "memcpy("+varp->name()+", &(handlep__V->"+varp->name()+"), "+std::to_string(bytes)+");\n";
        } else {
            return "memcpy("+varp->name()+", handlep__V->"+varp->name()+", "+std::to_string(bytes)+");\n";
        }
    }

    void handleClock(AstVar* varp) {
        FileLine* fl = varp->fileline();
        handleInput(varp);
        m_seqPortsp->addText(fl, "input bit "+sizedSvName(varp)+"\n");
        m_seqParamsp->addText(fl, varp->name()+"\n");
        m_clkSensp->addText(fl, "edge("+varp->name()+")");
        m_cSeqParamsp->addText(fl, typedCppInName(varp)+"\n");
        m_cSeqClksp->addText(fl, cInputConnection(varp));
    }

    void handleDataInput(AstVar* varp) {
        FileLine* fl = varp->fileline();
        handleInput(varp);
        m_comboPortsp->addText(fl, "input bit "+sizedSvName(varp)+"\n");
        m_comboParamsp->addText(fl, varp->name()+"\n");
        m_comboIgnorePortsp->addText(fl, "input bit "+sizedSvName(varp)+"\n");
        m_comboIgnoreParamsp->addText(fl, varp->name()+"\n");
        m_cComboParamsp->addText(fl, typedCppInName(varp)+"\n");
        m_cComboInsp->addText(fl, cInputConnection(varp));
        m_cIgnoreParamsp->addText(fl, typedCppInName(varp)+"\n");
    }

    void handleInput(AstVar* varp) {
        FileLine* fl = varp->fileline();
        m_modPortsp->addText(fl, "input "+sizedSvName(varp)+"\n");
    }

    void handleOutput(AstVar* varp) {
        FileLine* fl = varp->fileline();
        m_modPortsp->addText(fl, "output logic "+sizedSvName(varp)+"\n");
        m_comboPortsp->addText(fl, "output bit "+sizedSvName(varp)+"\n");
        m_comboParamsp->addText(fl, varp->name()+"_combo__V\n");
        m_seqPortsp->addText(fl, "output bit "+sizedSvName(varp)+"\n");
        m_seqParamsp->addText(fl, varp->name()+"_tmp__V\n");
        m_comboDeclsp->addText(fl, "logic "+sizedSvName(varp)+"_combo__V;\n");
        m_seqDeclsp->addText(fl, "logic "+sizedSvName(varp)+"_seq__V;\n");
        m_tmpDeclsp->addText(fl, "logic "+sizedSvName(varp)+"_tmp__V;\n");
        m_nbAssignsp->addText(fl, varp->name()+"_seq__V <= "+varp->name()+"_tmp__V;\n");
        m_seqAssignsp->addText(fl, varp->name()+" = "+varp->name()+"_seq__V;\n");
        m_comboAssignsp->addText(fl, varp->name()+" = "+varp->name()+"_combo__V;\n");
        m_cComboParamsp->addText(fl, typedCppOutName(varp)+"\n");
        m_cComboOutsp->addText(fl, cOutputConnection(varp));
        m_cSeqParamsp->addText(fl, typedCppOutName(varp)+"\n");
        m_cSeqOutsp->addText(fl, cOutputConnection(varp));
    }

  public:
    explicit ProtectVisitor(AstNode* nodep):
        m_modProtected(false), m_vfilep(NULL), m_cfilep(NULL), m_modPortsp(NULL),
        m_comboPortsp(NULL), m_seqPortsp(NULL), m_comboIgnorePortsp(NULL), m_comboDeclsp(NULL),
        m_seqDeclsp(NULL), m_tmpDeclsp(NULL), m_clkSensp(NULL), m_comboIgnoreParamsp(NULL),
        m_seqParamsp(NULL), m_nbAssignsp(NULL), m_seqAssignsp(NULL), m_comboAssignsp(NULL),
        m_cComboParamsp(NULL), m_cComboInsp(NULL), m_cComboOutsp(NULL), m_cSeqParamsp(NULL),
        m_cSeqClksp(NULL), m_cSeqOutsp(NULL), m_cIgnoreParamsp(NULL),
        m_libName(v3Global.opt.dpiProtect()), m_topName(v3Global.opt.prefix())
    {
        iterate(nodep);
    }
};

//######################################################################
// DpiProtect class functions

void V3DpiProtect::protect() {
    UINFO(2,__FUNCTION__<<": "<<endl);
    ProtectVisitor visitor(v3Global.rootp());
}
