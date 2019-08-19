// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit DPI protected library files for tree
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
#include "V3EmitDpiProtect.h"
#include "V3File.h"
#include <list>

//######################################################################
// Internal EmitDpiProtect implementation

class EmitVWrapper {
  public:
    EmitVWrapper(AstNodeModule* modp):
        m_modp(modp),
        m_modName(v3Global.opt.prefix()),
        m_of(v3Global.opt.makeDir()+"/"+m_modName+".sv"),
        m_totalPorts(0)
    {
        UASSERT_OBJ(modp->isTop(), modp, "V3EmitDpiProtect::emitv on non-top-level module");
        discoverPorts();
        emit();
    }

  private:
    // TODO -- inouts?
    typedef std::list<AstVar*> VarList;
    VarList m_inputs;
    VarList m_outputs;
    int m_totalPorts;
    AstNodeModule* m_modp;
    string m_modName;
    V3OutVFile m_of;
    int m_currPort;

    void discoverPorts() {
        AstNode* stmtp = m_modp->stmtsp();
        while(stmtp) {
            AstVar* varp;
            if ((varp = VN_CAST(stmtp, Var)) && varp->isIO()) {
                if (varp->direction() == VDirection::INPUT) {
                    m_inputs.push_back(varp);
                    ++m_totalPorts;
                } else if (varp->direction() == VDirection::OUTPUT) {
                    m_outputs.push_back(varp);
                    ++m_totalPorts;
                } else {
                    varp->v3fatalSrc("Unsupported direction for --dpi-protect: "<<
                                     varp->direction().ascii());
                }
            }
            stmtp = stmtp->nextp();
        }
    }

    void emitComma() {
        if (++m_currPort != m_totalPorts) m_of.puts(",");
        m_of.puts("\n");
    }

    void emitDpiParameters(VarList& ports) {
            //, int a, bit clk, output int x);
        for (VarList::iterator it = ports.begin(); it != ports.end(); ++it) {
            AstVar* varp = *it;

            if (varp->direction() == VDirection::OUTPUT) m_of.puts("output ");
            // TODO -- consider passing 4-state through
            m_of.puts("bit ");
            int width;
            if ((width = varp->width()) > 1) m_of.puts("["+std::to_string(width-1)+":0] ");

            m_of.puts((*it)->name());

            emitComma();
            // TODO -- next
        }
    }

    void emitPorts(VarList& ports) {
        for (VarList::iterator it = ports.begin(); it != ports.end(); ++it) {
            AstVar* varp = *it;

            if (varp->direction() == VDirection::INPUT) {
                m_of.puts("input ");
            } else {
                m_of.puts("output logic ");
            }

            if (varp->isSigned()) m_of.puts("signed ");
            // TODO -- check for non-packabe types (e.g. string)?
            int width;
            if ((width = varp->width()) > 1) m_of.puts("["+std::to_string(width-1)+":0] ");

            m_of.puts((*it)->name());

            emitComma();
        }
    }

    void emit() {
        m_of.putsHeader();
        m_of.puts("// Wrapper module for DPI protected library\n");
        m_of.puts("// This file requires lib"+m_modName+".so to operate\n");
        m_of.puts("// See instructions in your simulator for how to load DPI libraries\n");

        // TODO -- figure out how the formatters work
        m_of.puts("module "+m_modName+"(\n");
        m_currPort = 0;
        emitPorts(m_inputs);
        emitPorts(m_outputs);
        m_of.puts(");\n\n");

        m_of.puts("import \"DPI-C\" function chandle create_dpi_prot_"+
                  m_modName+" (string scope);\n");
        // TODO -- break up setters, eval and maybe getters
        m_of.puts("import \"DPI-C\" function void eval_dpi_prot_"+m_modName+" (\n");
        m_of.puts("chandle handle\n");
        m_currPort = 0;
        emitDpiParameters(m_inputs);
        emitDpiParameters(m_outputs);
        m_of.puts(")\n");
        m_of.puts("import \"DPI-C\" function void final_dpi_prot_"+
                  m_modName+" (chandle handle);\n\n");

        m_of.puts("chandle handle;\n");
        m_of.puts("string scope;\n");
        m_of.puts("initial foo = create_foo($sformatf(\"%m\"));\n");
        m_of.puts("final final_foo(foo);\n\n");

        // TODO -- try to understand clocks and be smarter here
        m_of.puts("always @(*) eval_foo(foo, a, clk, x);\n\n");

        m_of.puts("endmodule\n");
    }
};

//######################################################################
// EmitDpiProtect class functions

void V3EmitDpiProtect::emitv() {
    UINFO(2,__FUNCTION__<<": "<<endl);
    // Process each module in turn
    // TODO -- does this even work for multiple top-levels?
    for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
         nodep; nodep = VN_CAST(nodep->nextp(), NodeModule)) {
        EmitVWrapper wrapper(nodep);
    }
}
