// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Decompose clock arrays
//
// Code available from: http://www.veripool.org/verilator
//
//*************************************************************************
//
// Copyright 2003-2015 by Todd Strader.  This program is free software; you can
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
// ClkVector TRANSFORMATIONS:
// TODO -- fill this in
// Check for clock arrays that need to be decomposed:
//   * RHS is clker
//   * LHS width > 1
//*************************************************************************

#include "config_build.h"
#include "verilatedos.h"
#include <cstdio>
#include <cstdarg>
#include <unistd.h>
#include <map>

#include "V3ClkVector.h"
#include "V3Global.h"
#include "V3Ast.h"
#include "V3AstNodes.h"

// TYPES
typedef map<int, AstVarScope*> ClkSourceMap;
// TODO -- do I need this class, or just the ClkSourceMap?
class ClkVectorInfo {
public:
    ClkSourceMap            m_clkSourceMap;         // Clock source map
};
typedef map<AstVarScope*, ClkVectorInfo> ClkVectorMap;

//######################################################################
// ClkVector state

class ClkVectorState {
private:

    // Members
    ClkVectorMap        m_clkVectorMap;         // Clock vector map

public:
    // METHODS
    static int debug() {
	static int level = -1;
	if (VL_UNLIKELY(level < 0)) level = v3Global.opt.debugSrcLevel(__FILE__);
	return level;
    }

    // ACCESSORS
    ClkVectorMap* clkVectorMapp() { return &m_clkVectorMap; }
};

//======================================================================

class ClkVectorFindVisitor : public AstNVisitor {
    // STATE
    ClkVectorState*             m_statep;               // Clock vector pass state
    AstVarScope*                m_lhsVector;            // LHS vector
    int                         m_rhsOffset;            // RHS offset for concatenation

    // METHODS
    int debug() { return ClkVectorState::debug(); }

    // VISITs
    virtual void visit(AstAssignW* nodep, AstNUser*) {
        UINFO(5,nodep<<endl);
//	if (debug()>=9) nodep->dumpTree(cout,"-assignw: ");
        int lhsWidth = nodep->lhsp()->dtypep()->width();
        if (lhsWidth <= 1)
            return;

        m_lhsVector = nodep->lhsp()->castVarRef()->varScopep();
        m_rhsOffset = 0;
        nodep->rhsp()->iterate(*this);
        m_lhsVector = NULL;
    }

    // TODO - others
    virtual void visit(AstConcat* nodep, AstNUser*) {
        UINFO(5,nodep<<endl);
        nodep->rhsp()->iterate(*this);
        nodep->lhsp()->iterate(*this);
    }

    virtual void visit(AstVarRef* nodep, AstNUser*) {
        UINFO(5,nodep<<endl);
        if (nodep->varp()->attrClocker() == AstVarAttrClocker::CLOCKER_YES && m_lhsVector) {
            // TODO - handle decomposing into clockers > 1 bit?
            if (nodep->varp()->width() > 1) {
                UINFO(9,"Clocker > 1 bit, not decomposing"<<endl);
            } else {
                (*m_statep->clkVectorMapp())[m_lhsVector].m_clkSourceMap[m_rhsOffset] = nodep->varScopep();
                UINFO(9,"CLOCK VECTOR: "<<m_lhsVector<<" ("<<m_rhsOffset<<") => "<<nodep->varScopep()<<endl);
                // TODO -- do we even need a warning?
//	        nodep->v3warn(CLKVECTOR,"Clock array found: "<<nodep->prettyName());
	    }
        }
        m_rhsOffset += nodep->varp()->width();
    }

    virtual void visit(AstNode* nodep, AstNUser*) {
	// Default: Just iterate
	nodep->iterateChildren(*this);
    }

public:
    // CONSTUCTORS
    ClkVectorFindVisitor(AstNetlist* rootp, ClkVectorState* statep) {
	UINFO(4,__FUNCTION__<<": "<<endl);
        m_statep = statep;
        m_lhsVector = NULL;
        m_rhsOffset = 0;
	//
	rootp->accept(*this);
    }
    virtual ~ClkVectorFindVisitor() {}
};

//======================================================================

class ClkVectorReplaceVisitor : public AstNVisitor {
    // STATE
    ClkVectorState*             m_statep;               // Clock vector pass state
    AstVarRef*                  m_lhs;                  // Candidate for replacement

    // METHODS
    int debug() { return ClkVectorState::debug(); }

    // VISITs
    virtual void visit(AstAssignW* nodep, AstNUser*) {
        UINFO(5,nodep<<endl);
        AstVarRef * varrefp = nodep->lhsp()->castVarRef();
        if (varrefp == NULL)
            return;
        // TODO - handle > 1 bit RHSs?
        if (varrefp->varp()->width() == 1) {
            m_lhs = varrefp;
            nodep->rhsp()->iterate(*this);
            m_lhs = NULL;
        }
    }

    virtual void visit(AstSel* nodep, AstNUser*) {
        UINFO(5,nodep<<endl);
        if (m_lhs == NULL)
            return;
        AstVarRef* fromVarrefp = nodep->fromp()->castVarRef();
        if (fromVarrefp == NULL)
            return;
        // TODO - handle > 1 bit RHSs?
        if (nodep->widthp()->castConst() == NULL || nodep->widthp()->castConst()->toUInt() != 1)
            return;
        if (nodep->lsbp()->castConst() == NULL)
            return;
        uint32_t vectorIndex = nodep->lsbp()->castConst()->toUInt();
        AstVarScope* lhsVarScopep = m_lhs->varScopep();
        AstVarScope* replacementVarScopep = NULL;
        ClkVectorMap::iterator vectorIt = m_statep->clkVectorMapp()->find(fromVarrefp->varScopep());
        UINFO(9,"Checking clkVectorMap for: "<<fromVarrefp->varScopep()<<endl);
        if (vectorIt == m_statep->clkVectorMapp()->end())
            return;
        ClkSourceMap::iterator sourceIt = vectorIt->second.m_clkSourceMap.find(vectorIndex);
        UINFO(9,"Checking clkSourceMap for "<<vectorIndex<<endl);
        if (sourceIt == vectorIt->second.m_clkSourceMap.end())
            return;
        UINFO(9,"Replacement clock: "<<sourceIt->second<<endl);
        fromVarrefp->varScopep(sourceIt->second);
        nodep->replaceWith(fromVarrefp->unlinkFrBack());
        pushDeletep(nodep); VL_DANGLING(nodep);
    }

    virtual void visit(AstNode* nodep, AstNUser*) {
	// Default: Just iterate
	nodep->iterateChildren(*this);
    }

public:
    // CONSTUCTORS
    ClkVectorReplaceVisitor(AstNetlist* rootp, ClkVectorState* statep) {
	UINFO(4,__FUNCTION__<<": "<<endl);
        m_statep = statep;
        m_lhs = NULL;
	//
	rootp->accept(*this);
    }
    virtual ~ClkVectorReplaceVisitor() {}
};

//######################################################################
// Task class functions

void V3ClkVector::clkVectorDecompose(AstNetlist* nodep) {
    UINFO(2,__FUNCTION__<<": "<<endl);
    // TODO - multiple passes
//    do {
        ClkVectorState state;
        ClkVectorFindVisitor findVisitor (nodep, &state);
        ClkVectorReplaceVisitor replaceVisitor (nodep, &state);
//    } while (!state.clkVectorMapp->empty());
    V3Global::dumpCheckGlobalTree("clkvector.tree", 0, v3Global.opt.dumpTreeLevel(__FILE__) >= 3);
}
