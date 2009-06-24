//----------------------------------------------------------------------------
/** @file VCSet.cpp
 */
//----------------------------------------------------------------------------

#include "Hex.hpp"
#include "ChangeLog.hpp"
#include "VCSet.hpp"
#include "VC.hpp"
#include "VCList.hpp"

using namespace benzene;

//----------------------------------------------------------------------------

VCSet::VCSet(const ConstBoard& brd, HexColor color)
    : m_brd(&brd), 
      m_color(color)
{
    int softlimit_full = 25;
    int softlimit_semi = 50;

    // Create a list for each valid pair; also create lists
    // for pairs (x,x) for ease of use later on. These lists
    // between the same point will also be empty.
    for (BoardIterator y(m_brd->EdgesAndInterior()); y; ++y) {
        for (BoardIterator x(m_brd->EdgesAndInterior()); x; ++x) {
            m_vc[VC::FULL][*x][*y] = 
            m_vc[VC::FULL][*y][*x] = 
                new VCList(*y, *x, softlimit_full);

            m_vc[VC::SEMI][*x][*y] = 
            m_vc[VC::SEMI][*y][*x] = 
                new VCList(*y, *x, softlimit_semi);

            if (*x == *y) break;
        }
    }
}

VCSet::VCSet(const VCSet& other)
    : m_brd(other.m_brd),
      m_color(other.m_color)
{
    AllocateAndCopyLists(other);
}

void VCSet::operator=(const VCSet& other)
{
    m_brd = other.m_brd;
    m_color = other.m_color;
    FreeLists();
    AllocateAndCopyLists(other);
}

void VCSet::AllocateAndCopyLists(const VCSet& other)
{
    for (BoardIterator y = m_brd->EdgesAndInterior(); y; ++y) {
        for (BoardIterator x = m_brd->EdgesAndInterior(); x; ++x) {
            m_vc[VC::FULL][*x][*y] = 
            m_vc[VC::FULL][*y][*x] = 
                new VCList(*other.m_vc[VC::FULL][*y][*x]);

            m_vc[VC::SEMI][*x][*y] = 
            m_vc[VC::SEMI][*y][*x] = 
                new VCList(*other.m_vc[VC::SEMI][*y][*x]);

            if (*x == *y) break;
        }
    }
}

VCSet::~VCSet()
{
    FreeLists();
}

void VCSet::FreeLists()
{
    for (BoardIterator y = m_brd->EdgesAndInterior(); y; ++y) {
        for (BoardIterator x = m_brd->EdgesAndInterior(); x; ++x) {
            for (int i=0; i<VC::NUM_TYPES; ++i)
                delete m_vc[i][*x][*y];
            if (*x == *y) break;
        }
    }
}

//----------------------------------------------------------------------------

bool VCSet::Exists(HexPoint x, HexPoint y, VC::Type type) const
{
    return !m_vc[type][x][y]->empty();
}

bool VCSet::SmallestVC(HexPoint x, HexPoint y, 
                             VC::Type type, VC& out) const
{
    if (!Exists(x, y, type)) 
        return false;
    out = *m_vc[type][x][y]->begin();
    return true;
}

void VCSet::VCs(HexPoint x, HexPoint y, VC::Type type,
                      std::vector<VC>& out) const
{
    out.clear();
    const VCList* who = m_vc[type][x][y];
    for (VCList::const_iterator it(who->begin()); it != who->end(); ++it)
        out.push_back(*it);
}

//----------------------------------------------------------------------------

void VCSet::SetSoftLimit(VC::Type type, int limit)
{
    for (BoardIterator y(m_brd->EdgesAndInterior()); y; ++y) {
        for (BoardIterator x(m_brd->EdgesAndInterior()); *x != *y; ++x)
            m_vc[type][*x][*y]->setSoftLimit(limit);
    }
}

void VCSet::Clear()
{
    for (BoardIterator y(m_brd->EdgesAndInterior()); y; ++y) {
        for (BoardIterator x(m_brd->EdgesAndInterior()); *x != *y; ++x) {
            for (int i=0; i<VC::NUM_TYPES; ++i) {
                m_vc[i][*x][*y]->clear();
            }
        }
    }
}

void VCSet::Revert(ChangeLog<VC>& log)
{
    while (!log.empty()) {
        int action = log.topAction();
        if (action == ChangeLog<VC>::MARKER) {
            log.pop();
            break;
        }
        
        VC vc(log.topData());
        log.pop();

        VCList* list = m_vc[vc.type()][vc.x()][vc.y()];
        if (action == ChangeLog<VC>::ADD) {
#ifdef NDEBUG
            list->remove(vc, 0);
#else
            HexAssert(list->remove(vc, 0));
#endif
        } else if (action == ChangeLog<VC>::REMOVE) {
            list->simple_add(vc);
        } else if (action == ChangeLog<VC>::PROCESSED) {
            VCList::iterator it = list->find(vc);
            HexAssert(it != list->end());
            HexAssert(it->processed());
            it->setProcessed(false);
        }
    }    
}

//----------------------------------------------------------------------------

bool VCSet::operator==(const VCSet& other) const
{
    for (BoardIterator x(m_brd->EdgesAndInterior()); x; ++x) 
    {
        for (BoardIterator y(m_brd->EdgesAndInterior()); *y != *x; ++y) 
        {
            const VCList& full1 = *m_vc[VC::FULL][*x][*y];
            const VCList& full2 = *other.m_vc[VC::FULL][*x][*y];
            if (full1 != full2) return false;

            const VCList& semi1 = *m_vc[VC::SEMI][*x][*y];
            const VCList& semi2 = *other.m_vc[VC::SEMI][*x][*y];
            if (semi1 != semi2) return false;
        }
    }
    return true;
}
 
bool VCSet::operator!=(const VCSet& other) const
{
    return !operator==(other);
}

//----------------------------------------------------------------------------

bitset_t VCSetUtil::ConnectedTo(const VCSet& con, const Groups& groups, 
                              HexPoint x, VC::Type type)
{
    bitset_t ret;
    const StoneBoard& brd = groups.Board();
    HexColorSet not_other = HexColorSetUtil::ColorOrEmpty(con.Color());
    for (BoardIterator y(brd.Stones(not_other)); y; ++y) 
        if (con.Exists(groups.CaptainOf(x), groups.CaptainOf(*y), type))
            ret.set(*y);
    return ret;
}

void VCSetUtil::NumActiveVCSet(const VCSet& con, const Groups& groups, 
                               int& fulls, int& semis)
{
    fulls = semis = 0;
    HexColorSet not_other = HexColorSetUtil::ColorOrEmpty(con.Color());
    for (GroupIterator x(groups, not_other); x; ++x) 
    {
        for (GroupIterator y(groups, not_other); &*y != &*x; ++y) 
        {
            fulls += con.GetList(VC::FULL, x->Captain(), y->Captain()).size();
            semis += con.GetList(VC::SEMI, x->Captain(), y->Captain()).size();
        }
    }
}

bool VCSetUtil::EqualOnGroups(const VCSet& c1, const VCSet& c2,
                              const Groups& groups)
{
    if (c1.Color() != c2.Color())
        return false;
    if (c1.Board() != c2.Board())
        return false;

    HexColorSet not_other = HexColorSetUtil::ColorOrEmpty(c1.Color());    
    for (GroupIterator x(groups, not_other); x; ++x) 
    {
        HexPoint xc = x->Captain();
        for (GroupIterator y(groups, not_other); &*y != &*x; ++y) 
        {
            HexPoint yc = y->Captain();
            if (c1.GetList(VC::FULL, xc, yc) != c2.GetList(VC::FULL, xc, yc)) 
            {
                std::cout << "FULL " << xc << ", " << yc << "\n";
                std::cout << c1.GetList(VC::FULL, xc, yc).dump() << '\n';
                std::cout << "==============\n";
                std::cout << c2.GetList(VC::FULL, xc, yc).dump() << '\n';
                return false;
            }
            if (c1.GetList(VC::SEMI, xc, yc) != c2.GetList(VC::SEMI, xc, yc))
            {
                std::cout << "SEMI " << xc << ", " << yc << "\n";
                std::cout << c1.GetList(VC::SEMI, xc, yc).dump() << '\n';
                std::cout << "==============\n";
                std::cout << c2.GetList(VC::SEMI, xc, yc).dump() << '\n';
                return false;
            }
        }
    }
    return true;
}

//----------------------------------------------------------------------------
