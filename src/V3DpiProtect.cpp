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
    AstTextBlock* m_modPortsp;
    AstTextBlock* m_comboPortsp;
    AstTextBlock* m_seqPortsp;
    AstTextBlock* m_comboIgnorePortsp;
    AstTextBlock* m_comboDeclsp;
    AstTextBlock* m_seqDeclsp;
    AstTextBlock* m_comboParamsp;
    string m_libName;

    // VISITORS
    virtual void visit(AstNetlist* nodep) {
        m_vfilep = new AstVFile(nodep->fileline(),
                v3Global.opt.makeDir()+"/"+m_libName+".sv");
        nodep->addFilesp(m_vfilep);
        iterateChildren(nodep);
    }

    virtual void visit(AstNodeModule* nodep) {
        if (m_modProtected)
            v3Global.rootp()->v3error("Unsupported: --dpi-protect with multiple"
                                      " top-level modules");
        m_modProtected = true;
        createSvModule(nodep);

        iterateChildren(nodep);
    }

    void createSvModule(AstNodeModule* modp) {
        FileLine* fl = modp->fileline();
        // Comments
        AstTextBlock* txtp = new AstTextBlock(fl,
                "// Wrapper module for DPI protected library\n");
        txtp->addText(fl,
                       "// This module requires lib"+m_libName+
                       ".a or lib"+m_libName+".so to "
                       "work\n");
        txtp->addText(fl,
                       "// See instructions in your simulator for how to use "
                       "DPI libraries\n");

        // Module declaration
        m_modPortsp = new AstTextBlock(fl,
                       "module "+m_libName+" (\n",
                       false, true);
        txtp->addTextp(m_modPortsp);
        txtp->addText(fl, ");\n\n");

        // DPI declarations
        txtp->addText(fl, "import \"DPI-C\" function chandle "
                       "create_dpi_prot_"+m_libName+" (string scope);\n");
        m_comboPortsp = new AstTextBlock(fl,
                       "import \"DPI-C\" function void combo_update_dpi_prot_"+
                       m_libName+" (\n", false, true);
        m_comboPortsp->addText(fl, "chandle handle\n");
        txtp->addTextp(m_comboPortsp);
        txtp->addText(fl, ");\n");
        m_seqPortsp = new AstTextBlock(fl,
                       "import \"DPI-C\" function void seq_update_dpi_prot_"+
                       m_libName+" (\n", false, true);
        m_seqPortsp->addText(fl, "chandle handle\n");
        txtp->addTextp(m_seqPortsp);
        txtp->addText(fl, ");\n");
        m_comboIgnorePortsp = new AstTextBlock(fl,
                       "import \"DPI-C\" function void combo_ignore_dpi_prot_"+
                       m_libName+" (\n", false, true);
        m_comboIgnorePortsp->addText(fl, "chandle handle\n");
        txtp->addTextp(m_comboIgnorePortsp);
        txtp->addText(fl, ");\n");
        txtp->addText(fl, "import \"DPI-C\" function void "
                       "final_dpi_prot_"+m_libName+" (chandle handle);\n\n");

        // Local variables
        txtp->addText(fl, "chandle handle;\n\n");
        m_comboDeclsp = new AstTextBlock(fl, "");
        txtp->addTextp(m_comboDeclsp);
        m_seqDeclsp = new AstTextBlock(fl, "");
        txtp->addTextp(m_seqDeclsp);
        txtp->addText(fl, "\ntime last_combo_time\n");
        txtp->addText(fl, "time last_seq_time\n\n");

        // Initial
        txtp->addText(fl, "initial handle = create_dpi_prot_"+
                       m_libName+"($sformatf(\"%m\"));\n\n");

        // Combinatorial process
        m_comboParamsp = new AstTextBlock(fl, "always @(*) begin\n"
                       "combo_update_dpi_prot_"+m_libName+"(\n",
                       false, true);
        m_comboParamsp->addText(fl, "handle\n");
        txtp->addTextp(m_comboParamsp);
        txtp->addText(fl, ");\nlast_combo_time = $time;\nend\n\n");

        // Sequential process
//        m_clkSensp = new AstTextBlock(fl, "always @(", false, true);
//        txtp->addTextp(m_clkSensp);
        // TODO ...

        txtp->addText(fl, "endmodule\n");
        m_vfilep->tblockp(txtp);
    }

    virtual void visit(AstVar* nodep) {
        if (!nodep->isIO()) return;
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
        int width;
        if ((width = varp->width()) > 1)
            result += "["+std::to_string(width-1)+":0] ";
        result += varp->name();
        return result;
    }

    void handleClock(AstVar* varp) {
        FileLine* fl = varp->fileline();
        handleInput(varp);
        m_seqPortsp->addText(fl, "bit "+sizedSvName(varp)+"\n");
    }

    void handleDataInput(AstVar* varp) {
        FileLine* fl = varp->fileline();
        handleInput(varp);
        m_comboPortsp->addText(fl, "bit "+sizedSvName(varp)+"\n");
        m_comboParamsp->addText(fl, varp->name()+"\n");
        m_comboIgnorePortsp->addText(fl, "bit "+sizedSvName(varp)+"\n");
    }

    void handleInput(AstVar* varp) {
        FileLine* fl = varp->fileline();
        m_modPortsp->addText(fl, "input "+sizedSvName(varp)+"\n");
    }

    void handleOutput(AstVar* varp) {
        FileLine* fl = varp->fileline();
        m_modPortsp->addText(fl, "output logic "+sizedSvName(varp)+"\n");
        m_comboPortsp->addText(fl, "output bit "+sizedSvName(varp)+"\n");
        m_comboParamsp->addText(fl, varp->name()+"\n");
        m_seqPortsp->addText(fl, "output bit "+sizedSvName(varp)+"\n");
        m_comboDeclsp->addText(fl, "logic "+sizedSvName(varp)+"_combo;\n");
        m_seqDeclsp->addText(fl, "logic "+sizedSvName(varp)+"_seq;\n");
    }

  public:
    explicit ProtectVisitor(AstNode* nodep) {
        m_modProtected = false;
        m_vfilep = NULL;
        m_modPortsp = NULL;
        m_comboPortsp = NULL;
        m_seqPortsp = NULL;
        m_comboIgnorePortsp = NULL;
        m_comboDeclsp = NULL;
        m_seqDeclsp = NULL;
        m_libName = v3Global.opt.dpiProtect();
        iterate(nodep);
    }
};

//######################################################################
// DpiProtect class functions

void V3DpiProtect::protect() {
    UINFO(2,__FUNCTION__<<": "<<endl);
    ProtectVisitor visitor(v3Global.rootp());
}
