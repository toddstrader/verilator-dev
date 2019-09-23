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
    typedef std::list<AstVar*> VarList;
    VarList m_inputs;
    VarList m_clocks;
    VarList m_outputs;
    bool m_modProtected;
    AstVFile* m_vfilep;

    // VISITORS
    virtual void visit(AstNetlist* nodep) {
        m_vfilep = new AstVFile(nodep->fileline(),
                v3Global.opt.makeDir()+"/"+v3Global.opt.dpiProtect()+".sv");
        nodep->addFilesp(m_vfilep);
        iterateChildren(nodep);
    }

    virtual void visit(AstNodeModule* nodep) {
        if (m_modProtected)
            v3Global.rootp()->v3error("Unsupported: --dpi-protect with multiple"
                                      " top-level modules");
        m_modProtected = true;
        FileLine* fl = nodep->fileline();
        AstModule* modp = new AstModule(fl, v3Global.opt.dpiProtect());
        modp->noPrefix(true);
        modp->addStmtp(new AstComment(fl,
                    "Wrapper module for DPI protected library"));
        modp->addStmtp(new AstComment(fl,
                    "This module requires lib"+v3Global.opt.dpiProtect()+
                    ".a or lib"+v3Global.opt.dpiProtect()+".so to "
                    "work"));
        modp->addStmtp(new AstComment(fl,
                    "See instructions in your simulator for how to use "
                    "DPI libraries"));
        m_vfilep->modp(modp);

        iterateChildren(nodep);
    }

    virtual void visit(AstVar* nodep) {
        if (!nodep->isIO()) return;
    }

    virtual void visit(AstNode* nodep) { }

  public:
    explicit ProtectVisitor(AstNode* nodep) {
        m_modProtected = false;
        m_vfilep = NULL;
        iterate(nodep);
    }
};

//######################################################################
// DpiProtect class functions

void V3DpiProtect::protect() {
    UINFO(2,__FUNCTION__<<": "<<endl);
    ProtectVisitor visitor(v3Global.rootp());
}
