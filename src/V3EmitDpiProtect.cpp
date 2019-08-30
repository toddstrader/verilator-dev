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
        m_libName(v3Global.opt.dpiProtect()),
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
    string m_libName;

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
        m_of(v3Global.opt.makeDir()+"/"+m_libName+".sv")
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

    void emitDpiParameterDecls(VarList& ports) {
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

    void emitDpiParameters(VarList& ports) {
        for (VarList::iterator it = ports.begin(); it != ports.end(); ++it) {
            AstVar* varp = *it;

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
        m_of.puts("// This file requires lib"+m_libName+".so to operate\n");
        m_of.puts("// See instructions in your simulator for how to load DPI libraries\n");

        // TODO -- figure out how the formatters work
        m_of.puts("module "+m_libName+"(\n");
        m_currPort = 0;
        emitPorts(m_inputs);
        emitPorts(m_outputs);
        m_of.puts(");\n\n");

        m_of.puts("import \"DPI-C\" function chandle create_dpi_prot_"+
                  m_libName+" (string scope);\n");
        // TODO -- break up setters, eval and maybe getters
        m_of.puts("import \"DPI-C\" function void eval_dpi_prot_"+m_libName+" (\n");
        m_of.puts("chandle handle,\n");
        m_currPort = 0;
        emitDpiParameterDecls(m_inputs);
        emitDpiParameterDecls(m_outputs);
        m_of.puts(");\n");
        m_of.puts("import \"DPI-C\" function void final_dpi_prot_"+
                  m_libName+" (chandle handle);\n\n");

        m_of.puts("chandle handle;\n");
        m_of.puts("string scope;\n\n");

        m_of.puts("initial handle = create_dpi_prot_"+m_libName+"($sformatf(\"%m\"));\n\n");

        // TODO -- try to understand clocks and be smarter here
        m_of.puts("always @(*) eval_dpi_prot_"+m_libName+"(\n");
        m_of.puts("handle,\n");
        m_currPort = 0;
        emitDpiParameters(m_inputs);
        emitDpiParameters(m_outputs);
        m_of.puts(");\n\n");

        m_of.puts("final final_dpi_prot_"+m_libName+"(handle);\n\n");

        m_of.puts("endmodule\n");
    }
};

class EmitCWrapper: public EmitWrapper {
  public:
    EmitCWrapper(AstNodeModule* modp):
        EmitWrapper(modp),
        m_of(v3Global.opt.makeDir()+"/"+m_libName+".cpp"),
        m_topName(v3Global.opt.prefix())
    {
        emit();
    }

  private:
    string m_topName;
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
            int bytes = (width + 7) / 8;
            // TODO -- cases to test all of this
            if (width == 1) {
                m_of.puts("handle->"+varp->name()+" = "+varp->name()+";\n");
            } else if (bytes <= sizeof(uint32_t)) {
                m_of.puts("handle->"+varp->name()+" = *"+varp->name()+";\n");
            } else if (bytes <= sizeof(uint64_t)) {
                m_of.puts("memcpy(&(handle->"+varp->name()+"), "+varp->name()+", "+std::to_string(bytes)+");\n");
            } else {
                m_of.puts("memcpy(handle->"+varp->name()+", "+varp->name()+", "+std::to_string(bytes)+");\n");
            }
        }
    }

    void emitOutputConnections() {
        for (VarList::iterator it = m_outputs.begin(); it != m_outputs.end(); ++it) {
            AstVar* varp = *it;

            int bytes = (varp->width() + 7) / 8;
            // TODO -- cases to test all of this
            if (bytes <= sizeof(uint32_t)) {
                m_of.puts("*"+varp->name()+" = handle->"+varp->name()+";\n");
            } else if (bytes <= sizeof(uint64_t)) {
                m_of.puts("memcpy("+varp->name()+", &(handle->"+varp->name()+"), "+std::to_string(bytes)+");\n");
            } else {
                m_of.puts("memcpy("+varp->name()+", handle->"+varp->name()+", "+std::to_string(bytes)+");\n");
            }
        }
    }

    void emit() {
        m_of.putsHeader();
        m_of.puts("// Wrapper class for DPI protected library\n\n");
        m_of.puts("#include \""+m_topName+".h\"\n");
        m_of.puts("#include \"svdpi.h\"\n\n");

        m_of.puts("extern \"C\" {\n\n");
        m_of.puts("void* create_dpi_prot_"+m_libName+" (const char* scope) {\n");
        // TODO -- something more friendly here
        m_of.puts("assert(sizeof(WData) == sizeof(svBitVecVal));\n");
        m_of.puts(m_topName+"* handle = new "+m_topName+"(scope);\n");
        m_of.puts("return handle;\n");
        m_of.puts("}\n\n");

        m_of.puts("void eval_dpi_prot_"+m_libName+" (\n");
        m_of.puts("void* ptr,\n");
        m_currPort = 0;
        emitDpiParameters(m_inputs);
        emitDpiParameters(m_outputs);
        m_of.puts(")\n");
        m_of.puts("{\n");
        m_of.puts(m_topName+"* handle = static_cast<"+m_topName+"*>(ptr);\n");
        emitInputConnections();
        m_of.puts("handle->eval();\n");
        emitOutputConnections();
        m_of.puts("}\n\n");

        m_of.puts("void final_dpi_prot_"+m_libName+" (void* ptr) {\n");
        m_of.puts(m_topName+"* handle = static_cast<"+m_topName+"*>(ptr);\n");
        m_of.puts("handle->final();\n");
        m_of.puts("delete handle;\n");
        m_of.puts("}\n\n");
        m_of.puts("}\n;");
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
