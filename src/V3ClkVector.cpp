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
// Check for clock arrays that need to be decomposed:
//   * RHS is clker
//   * LHS width > 1
// Turn these clock vectors into individual clock signals
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
typedef map<AstVarScope*, AstVarScope*> ClkMap;
typedef map<int, AstVarScope*> ClkSourceMap;
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
    ClkMap              m_clkMap;               // Clock replacement map

public:
    // METHODS
    static int debug() {
	static int level = -1;
	if (VL_UNLIKELY(level < 0)) level = v3Global.opt.debugSrcLevel(__FILE__);
	return level;
    }

    // ACCESSORS
    ClkVectorMap* clkVectorMapp() { return &m_clkVectorMap; }
    ClkMap* clkMapp() { return &m_clkMap; }
};

//======================================================================

class ClkVectorFindVisitor : public AstNVisitor {
    // STATE
    ClkVectorState*             m_statep;               // Clock vector pass state
    AstVarScope*                m_lhsVector;            // LHS vector
    int                         m_lhsOffset;            // Offset info LHS
    bool                        m_inLhs;                // Iterating through LHS

    // METHODS
    int debug() { return ClkVectorState::debug(); }

    // VISITs
    virtual void visit(AstAssignW* nodep, AstNUser*) {
        UINFO(5,nodep<<endl);
        m_inLhs = true;
        m_lhsOffset = 0;
        nodep->lhsp()->iterate(*this);
        m_inLhs = false;
        nodep->rhsp()->iterate(*this);
        m_lhsVector = NULL;
    }

    virtual void visit(AstSel* nodep, AstNUser*) {
        UINFO(5,nodep<<endl);
        if (m_inLhs && nodep->lsbp()->castConst()) {
            m_lhsOffset = nodep->lsbConst();
            nodep->fromp()->iterate(*this);
        }
    }

    virtual void visit(AstConcat* nodep, AstNUser*) {
        UINFO(5,nodep<<endl);
        nodep->rhsp()->iterate(*this);
        nodep->lhsp()->iterate(*this);
    }

    virtual void visit(AstVarRef* nodep, AstNUser*) {
        UINFO(5,nodep<<endl);
        if (m_inLhs) {
            int lhsWidth = nodep->dtypep()->width();
            if (lhsWidth <= 1)
                return;
            m_lhsVector = nodep->varScopep();
            return;
        }
        if (nodep->varp()->attrClocker() == AstVarAttrClocker::CLOCKER_YES && m_lhsVector) {
            if (nodep->varp()->width() > 1) {
                UINFO(9,"Clocker > 1 bit, not decomposing"<<endl);
            } else {
                (*m_statep->clkVectorMapp())[m_lhsVector].m_clkSourceMap[m_lhsOffset] = nodep->varScopep();
                UINFO(9,"CLOCK VECTOR: "<<m_lhsVector<<" ("<<m_lhsOffset<<") => "<<nodep->varScopep()<<endl);
	    }
        }
        m_lhsOffset += nodep->varp()->width();
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
        m_lhsOffset = 0;
        m_inLhs = false;
	//
	rootp->accept(*this);
    }
    virtual ~ClkVectorFindVisitor() {}
};

//======================================================================

class ClkVectorMapVisitor : public AstNVisitor {
    // STATE
    ClkVectorState*             m_statep;               // Clock vector pass state
    AstVarRef*                  m_lhs;                  // Candidate for replacement
    bool                        m_removeAssign;         // Remove the assignment

    // METHODS
    int debug() { return ClkVectorState::debug(); }

    // VISITs
    virtual void visit(AstAssignW* nodep, AstNUser*) {
        UINFO(5,nodep<<endl);
        AstVarRef * varrefp = nodep->lhsp()->castVarRef();
        if (varrefp == NULL)
            return;
        if (varrefp->varp()->width() == 1) {
            m_lhs = varrefp;
            m_removeAssign = false;
            nodep->rhsp()->iterate(*this);
            m_lhs = NULL;
            if (m_removeAssign) {
                UINFO(9,"Removing assignment: "<<nodep<<endl);
                nodep->unlinkFrBack()->deleteTree(); VL_DANGLING(nodep);
            }
        }
    }

    virtual void visit(AstSel* nodep, AstNUser*) {
        UINFO(5,nodep<<endl);
        if (m_lhs == NULL)
            return;
        AstVarRef* fromVarrefp = nodep->fromp()->castVarRef();
        if (fromVarrefp == NULL)
            return;
        if (nodep->widthp()->castConst() == NULL || nodep->widthConst() != 1)
            return;
        if (nodep->lsbp()->castConst() == NULL)
            return;
        uint32_t vectorIndex = nodep->lsbConst();
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
        UINFO(9,"Mapping clock: "<<lhsVarScopep<<" to: "<<sourceIt->second<<endl);
        (*m_statep->clkMapp())[lhsVarScopep] = sourceIt->second;
        m_removeAssign = true;
    }

    virtual void visit(AstNode* nodep, AstNUser*) {
	// Default: Just iterate
	nodep->iterateChildren(*this);
    }

public:
    // CONSTUCTORS
    ClkVectorMapVisitor(AstNetlist* rootp, ClkVectorState* statep) {
	UINFO(4,__FUNCTION__<<": "<<endl);
        m_statep = statep;
        m_lhs = NULL;
        m_removeAssign = false;
	//
	rootp->accept(*this);
    }
    virtual ~ClkVectorMapVisitor() {}
};

//======================================================================

class ClkVectorReplaceVisitor : public AstNVisitor {
    // STATE
    ClkVectorState*             m_statep;               // Clock vector pass state

    // METHODS
    int debug() { return ClkVectorState::debug(); }

    // VISITs
    virtual void visit(AstVarRef* nodep, AstNUser*) {
        UINFO(5,nodep<<endl);
        ClkMap::iterator replacementClk = m_statep->clkMapp()->find(nodep->varScopep());
        if (replacementClk != m_statep->clkMapp()->end()) {
            UINFO(9,"Replacing clock: "<<nodep->varScopep()<<" with: "<<replacementClk->second<<endl);
            nodep->varScopep(replacementClk->second);
            nodep->varp(replacementClk->second->varp());
            nodep->name(replacementClk->second->varp()->name());
        }
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
	//
	rootp->accept(*this);
    }
    virtual ~ClkVectorReplaceVisitor() {}
};

//######################################################################
// Task class functions

void V3ClkVector::clkVectorDecompose(AstNetlist* nodep) {
    UINFO(2,__FUNCTION__<<": "<<endl);
    ClkVectorState state;
    ClkVectorFindVisitor findVisitor (nodep, &state);
    ClkVectorMapVisitor mapVisitor (nodep, &state);
    ClkVectorReplaceVisitor replaceVisitor (nodep, &state);
    V3Global::dumpCheckGlobalTree("clkvector.tree", 0, v3Global.opt.dumpTreeLevel(__FILE__) >= 3);
}
