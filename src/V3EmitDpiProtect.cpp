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

class EmitWrapper {
  public:
    EmitWrapper(AstNodeModule* modp):
        m_modp(modp),
        m_modName(v3Global.opt.prefix()),
        m_totalPorts(0)
    {
        UASSERT_OBJ(modp->isTop(), modp, "V3EmitDpiProtect::emitv on non-top-level module");
        discoverPorts();
    }

  protected:
    // TODO -- inouts?
    typedef std::list<AstVar*> VarList;
    VarList m_inputs;
    VarList m_outputs;
    int m_totalPorts;
    AstNodeModule* m_modp;
    string m_modName;

  private:
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
};

class EmitVWrapper: public EmitWrapper {
  public:
    EmitVWrapper(AstNodeModule* modp):
        EmitWrapper(modp),
        m_of(v3Global.opt.makeDir()+"/"+m_modName+".sv")
    {
        emit();
    }

  private:
    int m_currPort;
    V3OutVFile m_of;

    void emitComma() {
        if (++m_currPort != m_totalPorts) m_of.puts(",");
        m_of.puts("\n");
    }

    void emitDpiParameters(VarList& ports) {
        for (VarList::iterator it = ports.begin(); it != ports.end(); ++it) {
            AstVar* varp = *it;

            if (varp->direction() == VDirection::OUTPUT) m_of.puts("output ");
            // TODO -- consider passing 4-state through (as randomized values?)
            m_of.puts("bit ");
            int width;
            if ((width = varp->width()) > 1) m_of.puts("["+std::to_string(width-1)+":0] ");

            m_of.puts((*it)->name());

            emitComma();
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
        m_of.puts("string scope;\n\n");

        m_of.puts("initial foo = create_dpi_prot_foo($sformatf(\"%m\"));\n\n");

        // TODO -- try to understand clocks and be smarter here
        m_of.puts("always @(*) eval_dpi_prot_foo(foo, a, clk, x);\n\n");

        m_of.puts("final final_dpi_prot_foo(foo);\n\n");

        m_of.puts("endmodule\n");
    }
};

class EmitCWrapper: public EmitWrapper {
  public:
    EmitCWrapper(AstNodeModule* modp):
        EmitWrapper(modp),
        m_of(v3Global.opt.makeDir()+"/"+m_modName+".cpp")
    {
        emit();
    }

  private:
    int m_currPort;
    V3OutCFile m_of;

    // TODO -- remote duplication
    void emitComma() {
        if (++m_currPort != m_totalPorts) m_of.puts(",");
        m_of.puts("\n");
    }

    void emitDpiParameters(VarList& ports) {
        for (VarList::iterator it = ports.begin(); it != ports.end(); ++it) {
            AstVar* varp = *it;

            int width;
            if (varp->direction() == VDirection::INPUT) {
                if ((width = varp->width()) > 1) {
                    m_of.puts("const svBitVecVal* ");
                } else {
                    m_of.puts("unsigned char ");
                }
            } else {
                if ((width = varp->width()) > 1) {
                    m_of.puts("svBitVecVal* ");
                } else {
                    m_of.puts("unsigned char* ");
                }
            }

            m_of.puts((*it)->name());

            emitComma();
        }
    }

    void emitInputConnections() {
        for (VarList::iterator it = m_inputs.begin(); it != m_inputs.end(); ++it) {
            AstVar* varp = *it;

            int width = varp->width();
            // TODO -- cases to test all of this
            if (width == 1) {
                m_of.puts("handle->"+varp->name()+" = "+varp->name()+";\n");
            } else if (width <= sizeof(uint32_t)) {
                m_of.puts("handle->"+varp->name()+" = *"+varp->name()+";\n");
            } else {
                int bytes = (width + 7) / 8;
                m_of.puts("memcpy(handle->"+varp->name()+", "+varp->name()+", "+std::to_string(bytes)+");\n");
            }
        }
    }

    void emitOutputConnections() {
        for (VarList::iterator it = m_inputs.begin(); it != m_inputs.end(); ++it) {
            AstVar* varp = *it;

            int width = varp->width();
            // TODO -- cases to test all of this
            if (width <= sizeof(uint32_t)) {
                m_of.puts("*"+varp->name()+" = handle->"+varp->name()+";\n");
            } else {
                int bytes = (width + 7) / 8;
                m_of.puts("memcpy("+varp->name()+", handle->"+varp->name()+", "+std::to_string(bytes)+");\n");
            }
        }
    }

    void emit() {
        m_of.putsHeader();
        m_of.puts("// Wrapper class for DPI protected library\n\n");
        m_of.puts("#include \"V"+m_modName+".h\"\n");
        // TODO -- this externs the functions, does that matter?
        // TODO -- how should we get this file?  should we run verilator on the
        //         SV wrapper or just build it during this step?
        m_of.puts("#include \"V"+m_modName+"__Dpi.h\"\n\n");

        m_of.puts("void* create_dpi_prot_"+m_modName+" (const char* scope) {\n");
        // TODO -- something more friendly here
        m_of.puts("assert(sizeof(WData) == sizeof(svBitVecVal));\n");
        m_of.puts("V"+m_modName+"* handle = new V"+m_modName+"(scope);\n");
        m_of.puts("return handle;\n");
        m_of.puts("}\n\n");

        m_of.puts("void eval_dpi_prot_"+m_modName+" (\n");
        m_of.puts("void* ptr,\n");
        m_currPort = 0;
        emitDpiParameters(m_inputs);
        emitDpiParameters(m_outputs);
        m_of.puts(")\n");
        m_of.puts("{\n");
        m_of.puts("V"+m_modName+"* handle = static_cast<V"+m_modName+"*>(ptr);\n");
        emitInputConnections();
        m_of.puts("handle->eval();\n");
        emitOutputConnections();
        m_of.puts("}\n\n");

        m_of.puts("void final_dpi_prot_"+m_modName+" (void* ptr) {\n");
        m_of.puts("V"+m_modName+"* handle = static_cast<V"+m_modName+"*>(ptr);\n");
        m_of.puts("handle->final();\n");
        m_of.puts("delete handle;\n");
        m_of.puts("}\n\n");

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
        EmitVWrapper vWrapper(nodep);
        EmitCWrapper cWrapper(nodep);
    }
}
