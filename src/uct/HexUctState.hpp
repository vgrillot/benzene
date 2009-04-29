//----------------------------------------------------------------------------
/** @file HexUctState.hpp
 */
//----------------------------------------------------------------------------

#ifndef HEXUCTSTATE_HPP
#define HEXUCTSTATE_HPP

#include "SgBlackWhite.h"
#include "SgPoint.h"
#include "SgUctSearch.h"

#include "HashMap.hpp"
#include "HexBoard.hpp"
#include "VC.hpp"

#include <boost/scoped_ptr.hpp>

_BEGIN_BENZENE_NAMESPACE_

class HexUctSearch;

//----------------------------------------------------------------------------

/** Black and white stones. */
struct HexUctStoneData
{
    bitset_t black;

    bitset_t white;

    bitset_t played;

    /** Creates empty stone set. */
    HexUctStoneData();

    /** Copies stones from board. */
    HexUctStoneData(const StoneBoard& brd);
};

inline HexUctStoneData::HexUctStoneData()
{
}

inline HexUctStoneData::HexUctStoneData(const StoneBoard& brd)
    : black(brd.getBlack()),
      white(brd.getWhite()),
      played(brd.getPlayed())
{
}

/** Data shared among all threads. */
struct HexUctSharedData
{
    HexColor root_to_play;
    HexPoint root_last_move_played;
    bitset_t root_consider;
    HexUctStoneData root_stones;

    HashMap<HexUctStoneData> stones;

    HexUctSharedData();
};

inline HexUctSharedData::HexUctSharedData()
    : stones(16)
{ 
}

//----------------------------------------------------------------------------

/** Interface for policies controlling move generation in the random
    play-out phase of HexUctSearch. */
class HexUctSearchPolicy
{
public:

    virtual ~HexUctSearchPolicy() { };

    /** Generate a move in the random play-out phase of
        HexUctSearch. */
    virtual HexPoint GenerateMove(PatternBoard& brd, HexColor color, 
                                  HexPoint lastMove) = 0;

    virtual void InitializeForRollout(const StoneBoard& brd) = 0;
};

//----------------------------------------------------------------------------

/** Thread state for HexUctSearch. */
class HexUctState : public SgUctThreadState
{
public: 

    /** Constructor.
        @param threadId The number of the thread. Needed for passing to
        constructor of SgUctThreadState.
        @param sch Parent Search object.
        @param treeUpdateRadius Pattern matching radius in tree.
        @param playoutUpdateRadius Pattern matching radius in playouts.
    */
    HexUctState(std::size_t threadId,
		HexUctSearch& sch,
                int treeUpdateRadius,
                int playoutUpdateRadius);

    virtual ~HexUctState();

    //-----------------------------------------------------------------------

    /** @name Pure virtual functions of SgUctThreadState */
    // @{

    float Evaluate();
    
    void Execute(SgMove move);

    void ExecutePlayout(SgMove move);
   
    bool GenerateAllMoves(std::size_t count, std::vector<SgMoveInfo>& moves);

    SgMove GeneratePlayoutMove(bool& skipRaveUpdate);
    
    void StartSearch();

    void TakeBackInTree(std::size_t nuMoves);

    void TakeBackPlayout(std::size_t nuMoves);

    SgBlackWhite ToPlay() const;

    // @} // @name

    //-----------------------------------------------------------------------

    /** @name Virtual functions of SgUctThreadState */
    // @{

    void GameStart();

    void StartPlayouts();

    virtual void StartPlayout();

    virtual void EndPlayout();

    // @} // @name

    //-----------------------------------------------------------------------

    const PatternBoard& Board() const;

    HexUctSearchPolicy* Policy();

    bool IsInPlayout() const;

    void Dump(std::ostream& out) const;

    /** Sets random policy (takes control of pointer) and deletes the
        old one, if it existed. */
    void SetPolicy(HexUctSearchPolicy* policy);
    
    HexPoint GetLastMovePlayed() const;

    HexColor GetColorToPlay() const;

private:

    bitset_t ComputeKnowledge();

    /** Frees policy if one is assigned; does nothing otherwise. */
    void FreePolicy();
    
    /** Executes a move. */
    void ExecuteTreeMove(HexPoint move);

    void ExecuteRolloutMove(HexPoint move);

    void ExecutePlainMove(HexPoint cell, int updateRadius);

    /** Assertion handler to dump the state of a HexUctState. */
    class AssertionHandler
        : public SgAssertionHandler
    {
    public:
        AssertionHandler(const HexUctState& state);

        void Run();

    private:
        const HexUctState& m_state;
    };

    AssertionHandler m_assertionHandler;

    /** Board used during search. */
    boost::scoped_ptr<PatternBoard> m_bd;

    /** Board used to compute knowledge. */
    boost::scoped_ptr<HexBoard> m_vc_brd;

    HexUctSharedData* m_shared_data;

    HexUctSearch& m_search;

    HexUctSearchPolicy* m_policy;
    
    /** Color to play next. */
    HexColor m_toPlay;

    /** @see HexUctSearch::TreeUpdateRadius() */
    int m_treeUpdateRadius;

    /** @see HexUctSearch::PlayoutUpdateRadius() */
    int m_playoutUpdateRadius;

    //----------------------------------------------------------------------

    /** True if in playout phase. */
    bool m_isInPlayout;

    /** Moves played in the tree. */
    PointSequence m_tree_sequence;

    /** Keeps track of last playout move made.
	Used for pattern-generated rollouts when call
	HexUctSearchPolicy. */
    HexPoint m_lastMovePlayed;

    /** Number of stones played since initial board position */
    int m_numStonesPlayed;

    /** True at the start of a game until the first move is played. */
    bool m_new_game;
};

inline const PatternBoard& HexUctState::Board() const
{
    return *m_bd;
}

inline HexUctSearchPolicy* HexUctState::Policy()
{
    return m_policy;
}

inline bool HexUctState::IsInPlayout() const
{
    return m_isInPlayout;
}

inline HexPoint HexUctState::GetLastMovePlayed() const
{
    return m_lastMovePlayed;
}

inline HexColor HexUctState::GetColorToPlay() const
{
    return m_toPlay;
}

//----------------------------------------------------------------------------

_END_BENZENE_NAMESPACE_

#endif // HEXUCTSTATE_HPP
