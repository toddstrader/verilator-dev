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
    bool m_modProtected;
    AstVFile* m_vfilep;
    AstCFile* m_cfilep;
    AstTextBlock* m_modPortsp;
    AstTextBlock* m_comboPortsp;
    AstTextBlock* m_seqPortsp;
    AstTextBlock* m_comboIgnorePortsp;
    AstTextBlock* m_comboDeclsp;
    AstTextBlock* m_seqDeclsp;
    AstTextBlock* m_tmpDeclsp;
    AstTextBlock* m_comboParamsp;
    AstTextBlock* m_clkSensp;
    AstTextBlock* m_comboIgnoreParamsp;
    AstTextBlock* m_seqParamsp;
    AstTextBlock* m_nbAssignsp;
    AstTextBlock* m_seqAssignsp;
    AstTextBlock* m_comboAssignsp;
    AstTextBlock* m_cppComboParamsp;
    AstTextBlock* m_cppComboInsp;
    AstTextBlock* m_cppComboOutsp;
    AstTextBlock* m_cppSeqParamsp;
    AstTextBlock* m_cppSeqClksp;
    AstTextBlock* m_cppSeqOutsp;
    AstTextBlock* m_cppIgnoreParamsp;
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
        if (m_modProtected)
            v3Global.rootp()->v3error("Unsupported: --dpi-protect with multiple"
                                      " top-level modules");
        m_modProtected = true;
        createSvFile(nodep->fileline());
        createCppFile(nodep->fileline());

        iterateChildren(nodep);
    }

    void addComment(AstTextBlock* txtp, FileLine* fl, const string& comment) {
        txtp->addNodep(new AstComment(fl, comment));
    }

    void createSvFile(FileLine* fl) {
        // Comments
        AstTextBlock* txtp = new AstTextBlock(fl);
        // TODO -- why do emitted comments keep indenting?
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
        txtp->addText(fl, "import \"DPI-C\" function chandle "
                       "create_dpi_prot_"+m_libName+" (string scope);\n");
        m_comboPortsp = new AstTextBlock(fl,
                       "import \"DPI-C\" function void combo_update_dpi_prot_"+
                       m_libName+" (\n", false, true);
        m_comboPortsp->addText(fl, "chandle handle\n");
        txtp->addNodep(m_comboPortsp);
        txtp->addText(fl, ");\n");
        m_seqPortsp = new AstTextBlock(fl,
                       "import \"DPI-C\" function void seq_update_dpi_prot_"+
                       m_libName+" (\n", false, true);
        m_seqPortsp->addText(fl, "chandle handle\n");
        txtp->addNodep(m_seqPortsp);
        txtp->addText(fl, ");\n");
        m_comboIgnorePortsp = new AstTextBlock(fl,
                       "import \"DPI-C\" function void combo_ignore_dpi_prot_"+
                       m_libName+" (\n", false, true);
        m_comboIgnorePortsp->addText(fl, "chandle handle\n");
        txtp->addNodep(m_comboIgnorePortsp);
        txtp->addText(fl, ");\n");
        txtp->addText(fl, "import \"DPI-C\" function void "
                       "final_dpi_prot_"+m_libName+" (chandle handle);\n\n");

        // Local variables
        txtp->addText(fl, "chandle handle;\n\n");
        m_comboDeclsp = new AstTextBlock(fl);
        txtp->addNodep(m_comboDeclsp);
        m_seqDeclsp = new AstTextBlock(fl);
        txtp->addNodep(m_seqDeclsp);
        m_tmpDeclsp = new AstTextBlock(fl);
        txtp->addNodep(m_tmpDeclsp);
        txtp->addText(fl, "\ntime last_combo_time;\n");
        txtp->addText(fl, "time last_seq_time;\n\n");

        // Initial
        txtp->addText(fl, "initial handle = create_dpi_prot_"+
                       m_libName+"($sformatf(\"%m\"));\n\n");

        // Combinatorial process
        m_comboParamsp = new AstTextBlock(fl, "always @(*) begin\n"
                       "combo_update_dpi_prot_"+m_libName+"(\n",
                       false, true);
        m_comboParamsp->addText(fl, "handle\n");
        txtp->addNodep(m_comboParamsp);
        txtp->addText(fl, ");\n");
        txtp->addText(fl, "last_combo_time = $time;\n");
        txtp->addText(fl, "end\n\n");

        // Sequential process
        m_clkSensp = new AstTextBlock(fl, "always @(", false, true);
        txtp->addNodep(m_clkSensp);
        txtp->addText(fl, ") begin\n");
        m_comboIgnoreParamsp = new AstTextBlock(fl, "combo_ignore_dpi_prot_"+
                                                m_libName+"(\n", false, true);
        m_comboIgnoreParamsp->addText(fl, "handle\n");
        txtp->addNodep(m_comboIgnoreParamsp);
        txtp->addText(fl, ");\n");
        m_seqParamsp = new AstTextBlock(fl, "seq_update_dpi_prot_"+m_libName+
                                        "(\n", false, true);
        m_seqParamsp->addText(fl, "handle\n");
        txtp->addNodep(m_seqParamsp);
        txtp->addText(fl, ");\n");
        m_nbAssignsp = new AstTextBlock(fl);
        m_nbAssignsp->addText(fl, "last_seq_time <= $time;\n");
        txtp->addNodep(m_nbAssignsp);
        txtp->addText(fl, "end\n\n");

        // Select between combinatorial and sequential results
        txtp->addText(fl, "always @(*) begin\n");
        m_seqAssignsp = new AstTextBlock(fl, "if (last_seq_time > "
                                         "last_combo_time) begin\n");
        txtp->addNodep(m_seqAssignsp);
        m_comboAssignsp = new AstTextBlock(fl, "end else begin\n");
        txtp->addNodep(m_comboAssignsp);
        txtp->addText(fl, "end\n");
        txtp->addText(fl, "end\n\n");

        // Final
        txtp->addText(fl, "final final_dpi_prot_"+m_libName+"(handle);\n\n");

        txtp->addText(fl, "endmodule\n");
        m_vfilep->tblockp(txtp);
    }

    void castPtr(FileLine* fl, AstTextBlock* txtp) {
        txtp->addText(fl, m_topName+"* handle = static_cast<"+m_topName+"*>(ptr);\n");
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
        txtp->addText(fl, "void* create_dpi_prot_"+m_libName+
                      " (const char* scope) {\n");
        txtp->addText(fl, "assert(sizeof(WData) == sizeof(svBitVecVal));\n");
        txtp->addText(fl, m_topName+"* handle = new "+m_topName+"(scope);\n");
        txtp->addText(fl, "return handle;\n");
        txtp->addText(fl, "}\n\n");

        // Updates
        m_cppComboParamsp = new AstTextBlock(fl, "void combo_update_dpi_prot_"+
                      m_libName+" (\n", false, true);
        m_cppComboParamsp->addText(fl, "void* ptr\n");
        txtp->addNodep(m_cppComboParamsp);
        txtp->addText(fl, ")\n");
        m_cppComboInsp = new AstTextBlock(fl, "{\n");
        castPtr(fl, m_cppComboInsp);
        txtp->addNodep(m_cppComboInsp);
        m_cppComboOutsp = new AstTextBlock(fl, "handle->eval();\n");
        txtp->addNodep(m_cppComboOutsp);
        txtp->addText(fl, "}\n\n");

        m_cppSeqParamsp = new AstTextBlock(fl, "void seq_update_dpi_prot_"+
                                           m_libName+" (\n", false, true);
        m_cppSeqParamsp->addText(fl, "void* ptr\n");
        txtp->addNodep(m_cppSeqParamsp);
        txtp->addText(fl, ")\n");
        m_cppSeqClksp = new AstTextBlock(fl, "{\n");
        castPtr(fl, m_cppSeqClksp);
        txtp->addNodep(m_cppSeqClksp);
        m_cppSeqOutsp = new AstTextBlock(fl, "handle->eval();\n");
        txtp->addNodep(m_cppSeqOutsp);
        txtp->addText(fl, "}\n\n");

        m_cppIgnoreParamsp = new AstTextBlock(fl, "void combo_ignore_dpi_prot_"+
                                              m_libName+"(\n", false, true);
        m_cppIgnoreParamsp->addText(fl, "void* ptr\n");
        txtp->addNodep(m_cppIgnoreParamsp);
        txtp->addText(fl, ")\n");
        txtp->addText(fl, "{ }\n\n");
        // Final
        txtp->addText(fl, "void* final_dpi_prot_"+m_libName+
                      " (void* ptr) {\n");
        castPtr(fl, txtp);
        txtp->addText(fl, "handle->final();\n");
        txtp->addText(fl, "delete handle;\n");
        txtp->addText(fl, "}\n\n");

        txtp->addText(fl, "}\n");
        m_cfilep->tblockp(txtp);
    }

    virtual void visit(AstVar* nodep) {
        if (!nodep->isIO()) return;
        if (VN_IS(nodep->dtypep(), UnpackArrayDType))
            nodep->v3fatalSrc("Unsupported: unpacked arrays with --dpi-protect");
        if (nodep->direction() == VDirection::INPUT) {
            // TODO -- What is the differnce between isUsedClock()
            //           and attrClocker()?  The latter shows up
            //           when --clk is specified, but the former
            //           is there regardless.
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
        if (width > 1)
            result += "["+std::to_string(width-1)+":0] ";
        result += varp->name();
        return result;
    }

    string typedCppInName(AstVar* varp) {
        string result;
        int width = varp->width();
        if (width > 1)
            result += "const svBitVecVal* ";
        else
            result += "unsigned char ";
        result += varp->name();
        return result;
    }

    string typedCppOutName(AstVar* varp) {
        string result;
        int width = varp->width();
        if (width > 1)
            result += "svBitVecVal* ";
        else
            result += "unsigned char* ";
        result += varp->name();
        return result;
    }

    string cppInputConnection(AstVar* varp) {
        int width = varp->width();
        int bytes = (width + 7) / 8;
        if (width == 1) {
            return "handle->"+varp->name()+" = "+varp->name()+";\n";
        } else if (bytes <= sizeof(uint32_t)) {
            return "handle->"+varp->name()+" = *"+varp->name()+";\n";
        } else if (bytes <= sizeof(uint64_t)) {
            return "memcpy(&(handle->"+varp->name()+"), "+varp->name()+", "+std::to_string(bytes)+");\n";
        } else {
            return "memcpy(handle->"+varp->name()+", "+varp->name()+", "+std::to_string(bytes)+");\n";
        }
    }

    string cppOutputConnection(AstVar* varp) {
        int bytes = (varp->width() + 7) / 8;
        if (bytes <= sizeof(uint32_t)) {
            return "*"+varp->name()+" = handle->"+varp->name()+";\n";
        } else if (bytes <= sizeof(uint64_t)) {
            return "memcpy("+varp->name()+", &(handle->"+varp->name()+"), "+std::to_string(bytes)+");\n";
        } else {
            return "memcpy("+varp->name()+", handle->"+varp->name()+", "+std::to_string(bytes)+");\n";
        }
    }

    void handleClock(AstVar* varp) {
        FileLine* fl = varp->fileline();
        handleInput(varp);
        m_seqPortsp->addText(fl, "input bit "+sizedSvName(varp)+"\n");
        m_seqParamsp->addText(fl, varp->name()+"\n");
        m_clkSensp->addText(fl, "edge("+varp->name()+")");
        m_cppSeqParamsp->addText(fl, typedCppInName(varp)+"\n");
        m_cppSeqClksp->addText(fl, cppInputConnection(varp));
    }

    void handleDataInput(AstVar* varp) {
        FileLine* fl = varp->fileline();
        handleInput(varp);
        m_comboPortsp->addText(fl, "input bit "+sizedSvName(varp)+"\n");
        m_comboParamsp->addText(fl, varp->name()+"\n");
        m_comboIgnorePortsp->addText(fl, "input bit "+sizedSvName(varp)+"\n");
        m_comboIgnoreParamsp->addText(fl, varp->name()+"\n");
        m_cppComboParamsp->addText(fl, typedCppInName(varp)+"\n");
        m_cppComboInsp->addText(fl, cppInputConnection(varp));
        m_cppIgnoreParamsp->addText(fl, typedCppInName(varp)+"\n");
    }

    void handleInput(AstVar* varp) {
        FileLine* fl = varp->fileline();
        m_modPortsp->addText(fl, "input "+sizedSvName(varp)+"\n");
    }

    void handleOutput(AstVar* varp) {
        FileLine* fl = varp->fileline();
        m_modPortsp->addText(fl, "output logic "+sizedSvName(varp)+"\n");
        m_comboPortsp->addText(fl, "output bit "+sizedSvName(varp)+"\n");
        m_comboParamsp->addText(fl, varp->name()+"_combo\n");
        m_seqPortsp->addText(fl, "output bit "+sizedSvName(varp)+"\n");
        m_seqParamsp->addText(fl, varp->name()+"_tmp\n");
        m_comboDeclsp->addText(fl, "logic "+sizedSvName(varp)+"_combo;\n");
        m_seqDeclsp->addText(fl, "logic "+sizedSvName(varp)+"_seq;\n");
        m_tmpDeclsp->addText(fl, "logic "+sizedSvName(varp)+"_tmp;\n");
        m_nbAssignsp->addText(fl, varp->name()+"_seq <= "+varp->name()+"_tmp;\n");
        m_seqAssignsp->addText(fl, varp->name()+" = "+varp->name()+"_seq;\n");
        m_comboAssignsp->addText(fl, varp->name()+" = "+varp->name()+"_combo;\n");
        m_cppComboParamsp->addText(fl, typedCppOutName(varp)+"\n");
        m_cppComboOutsp->addText(fl, cppOutputConnection(varp));
        m_cppSeqParamsp->addText(fl, typedCppOutName(varp)+"\n");
        m_cppSeqOutsp->addText(fl, cppOutputConnection(varp));
    }

  public:
    explicit ProtectVisitor(AstNode* nodep):
        m_modProtected(false), m_vfilep(NULL), m_cfilep(NULL), m_modPortsp(NULL),
        m_comboPortsp(NULL), m_seqPortsp(NULL), m_comboIgnorePortsp(NULL), m_comboDeclsp(NULL),
        m_seqDeclsp(NULL), m_tmpDeclsp(NULL), m_clkSensp(NULL), m_comboIgnoreParamsp(NULL),
        m_seqParamsp(NULL), m_nbAssignsp(NULL), m_seqAssignsp(NULL), m_comboAssignsp(NULL),
        m_cppComboParamsp(NULL), m_cppComboInsp(NULL), m_cppComboOutsp(NULL), m_cppSeqParamsp(NULL),
        m_cppSeqClksp(NULL), m_cppSeqOutsp(NULL), m_cppIgnoreParamsp(NULL),
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
