extern "C"
{
#include "llvm/AccMutRuntime/accmut_GoodVar.h"
#include "llvm/AccMutRuntime/accmut_arith_common.h"
extern int32_t goodvar_fork(int mutID);
extern int default_active_set[];
extern int forked_active_set[];
extern int forked_active_num;
extern void __accmut__register(RegMutInfo *rmi);
}

#if !defined(NDEBUG)
#define NDEBUG
#endif
// #undef NDEBUG

#include <cstdio>
#include <cassert>
#include <vector>
#include <map>
#include <stack>
#include <algorithm>

#define iflikely(x) if (__glibc_likely(x))
#define ifunlikely(x) if (__glibc_unlikely(x))

using std::vector;
using std::map;

typedef int32_t GoodVarIdType;

const GoodVarIdType BADVAR = -1;

typedef int32_t OpCodeType;

template<class OpType, class RetType>
using CalcType = RetType(OpCodeType, OpType, OpType);

typedef int32_t MutationIdType;
typedef int64_t ValueType;
#define SPLIT
#define USE_VECTOR
#define FORKED_BITSET
// #define DEBUG_OUTPUT
// #define USE_VECTOR_POOL_WITH_ARRAY
#ifdef USE_VECTOR_POOL_WITH_ARRAY
#ifndef SPLIT
#define SPLIT
#endif
#ifndef USE_VECTOR
#define USE_VECTOR
#endif
#endif
#ifdef USE_VECTOR
typedef vector<std::pair<MutationIdType, ValueType>> GoodVarType;
#else
typedef map<MutationIdType, ValueType> GoodVarType;
#endif

#ifdef DEBUG_OUTPUT
#define fprintf if (MUTATION_ID == 0) fprintf
#else
#define fprintf(...)
#endif

#ifdef USE_VECTOR_POOL_WITH_ARRAY
template <typename T>
class Pool {
    std::vector<T*> data;
    std::vector<size_t> flist;
public:
    Pool() {
        data.reserve(100);
    }
    size_t get() {
        size_t ret;
        if (flist.empty()) {
            data.push_back(new T());
            ret = data.size() - 1;
            size_t n = data.size();
            /*for (size_t i = 0; i < n; ++i) {
                data.push_back(new T());
                flist.push_back(data.size() - 1);
            }*/
        } else {
            ret = flist.back();
            flist.pop_back();
        }
#ifdef DEBUG_OUTPUT
        fprintf(stderr, "LIST: ");
        for (int i = 0; i < flist.size(); ++i) {
            fprintf(stderr, "%ld, ", flist[i]);
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "GOT: %ld, %ld\n", data.size(), ret);
#endif
        return ret;
    }
    void unget(size_t i) {
#ifdef DEBUG_OUTPUT
        fprintf(stderr, "UNGOT: %ld\n", i);
#endif
        // assert(data[i]->empty());
        flist.push_back(i);
    }
    T* operator[](const size_t s) {
#ifdef DEBUG_OUTPUT
        fprintf(stderr, "size: %ld\n", data[s]->size());
#endif
        return data[s];
    }
    ~Pool() {
        for (auto ptr : data) {
            delete ptr;
        }
    }
};
Pool<GoodVarType> gvpool;
#define MAX_GOODVAR 15
struct GoodVarTableType {
    GoodVarType *goodVarArray[MAX_GOODVAR] = {};
    // std::array<GoodVarType *, MAX_GOODVAR> idArray;
    size_t needRetArray[MAX_GOODVAR];
    int endPos = 0;
};
#else
typedef map<GoodVarIdType, GoodVarType> GoodVarTableType;
#endif
static std::stack<GoodVarTableType *> goodVarTableStack;
#define goodVarTable (*goodVarTableStack.top())


void __accmut__GoodVar_BBinit() {
#ifdef USE_VECTOR_POOL_WITH_ARRAY
#ifdef DEBUG_OUTPUT
    fprintf(stderr, "BBINIT: ");
#endif
    iflikely (goodVarTable.endPos == 0)
        return;
    for (int i = 0; i < goodVarTable.endPos; ++i) {
        size_t n = goodVarTable.needRetArray[i];
        fprintf(stderr, "%ld\n", n);
        gvpool[n]->clear();
        fprintf(stderr, "BBUNGOT: ");
        gvpool.unget(n);
    }
    memset(goodVarTable.goodVarArray, 0, sizeof(GoodVarType *) * MAX_GOODVAR);
    goodVarTable.endPos = 0;
#else
    goodVarTable.clear();
#endif
}


void __accmut__GoodVar_TablePush() {
    fprintf(stderr, "TABLEPUSH: ");
    goodVarTableStack.push(new GoodVarTableType());
    // fprintf(stderr, "%ld\n", goodVarTableStack.size());
}


void __accmut__GoodVar_TablePop() {
#ifdef USE_VECTOR_POOL_WITH_ARRAY
    fprintf(stderr, "TABLEPOP: ");
    __accmut__GoodVar_BBinit();
#endif
    delete goodVarTableStack.top();
    goodVarTableStack.pop();
}


#ifndef FORKED_BITSET

static inline bool isActive(MutationIdType id) {
    if (MUTATION_ID == 0) {
        return default_active_set[id] == 1;
    } else {
        int *forked_active_set_end = forked_active_set + forked_active_num;
        //fprintf(stderr, "act: %d\n", forked_active_num);
        return std::find(forked_active_set, forked_active_set_end, id) != forked_active_set_end;
    }
}

#else

static bool forked_active_bitset[MAXMUTNUM];

static inline bool isActive(MutationIdType id) {
    if (MUTATION_ID == 0) {
        return default_active_set[id] == 1;
    } else {
        return forked_active_bitset[id];
    }
}

#endif


static vector<MutationIdType> recent_set;
static vector<ValueType> temp_result;

#ifdef SPLIT

template<class OpType, class RetType>
static inline void process_T_calc_GoodVar_Left(OpCodeType op, OpType right, CalcType<OpType, RetType> calc,
                                               GoodVarType::const_iterator leftIt,
                                               GoodVarType::const_iterator leftEnd) {


#ifdef FORKED_BITSET

    for (int i = 0; i < forked_active_num; ++i) {
        forked_active_bitset[forked_active_set[i]] = true;
    }

#endif
    assert(std::is_sorted(leftIt, leftEnd, [](const std::pair<MutationIdType, ValueType> a,
                                              const std::pair<MutationIdType, ValueType> b) {
        return a.first < b.first;
    }));

    for (; leftIt != leftEnd;) {
        if (isActive(leftIt->first)) {
            recent_set.push_back(leftIt->first);
            temp_result.push_back((ValueType) calc(op, (OpType) leftIt->second, right));
        }
        ++leftIt;
    }

#ifdef FORKED_BITSET

    for (int i = 0; i < forked_active_num; ++i) {
        forked_active_bitset[forked_active_set[i]] = false;
    }

#endif
}


template<class OpType, class RetType>
static inline void process_T_calc_GoodVar_Right(OpCodeType op, OpType left, CalcType<OpType, RetType> calc,
                                                GoodVarType::const_iterator rightIt,
                                                GoodVarType::const_iterator rightEnd) {


#ifdef FORKED_BITSET

    for (int i = 0; i < forked_active_num; ++i) {
        forked_active_bitset[forked_active_set[i]] = true;
    }

#endif
    assert(is_sorted(rightIt, rightEnd, [](const std::pair<MutationIdType, ValueType> a,
                                           const std::pair<MutationIdType, ValueType> b) {
        return a.first < b.first;
    }));

    for (; rightIt != rightEnd;) {
        if (isActive(rightIt->first)) {
            recent_set.push_back(rightIt->first);
            temp_result.push_back((ValueType) calc(op, left, (OpType) rightIt->second));
        }
        ++rightIt;
    }

#ifdef FORKED_BITSET

    for (int i = 0; i < forked_active_num; ++i) {
        forked_active_bitset[forked_active_set[i]] = false;
    }

#endif
}


template<class OpType, class RetType>
static inline void process_T_calc_GoodVar(OpCodeType op, OpType left, OpType right, CalcType<OpType, RetType> calc,
                                          GoodVarType::const_iterator leftIt, GoodVarType::const_iterator leftEnd,
                                          GoodVarType::const_iterator rightIt, GoodVarType::const_iterator rightEnd) {
    fprintf(stderr, "calc\n");
#ifdef FORKED_BITSET

    for (int i = 0; i < forked_active_num; ++i) {
        forked_active_bitset[forked_active_set[i]] = true;
    }

#endif

    assert(is_sorted(leftIt, leftEnd, [](const std::pair<MutationIdType, ValueType> a,
                                         const std::pair<MutationIdType, ValueType> b) { return a.first < b.first; }));
    assert(is_sorted(rightIt, rightEnd, [](const std::pair<MutationIdType, ValueType> a,
                                           const std::pair<MutationIdType, ValueType> b) {
        return a.first < b.first;
    }));
    for (; leftIt != leftEnd && rightIt != rightEnd;) {
        if (leftIt->first < rightIt->first) {
            if (isActive(leftIt->first)) {
                recent_set.push_back(leftIt->first);
                temp_result.push_back((ValueType) calc(op, (OpType) leftIt->second, right));
            }
            ++leftIt;
        } else if (leftIt->first > rightIt->first) {
            if (isActive(rightIt->first)) {
                recent_set.push_back(rightIt->first);
                temp_result.push_back((ValueType) calc(op, left, (OpType) rightIt->second));
            }
            ++rightIt;
        } else {
            if (isActive(leftIt->first)) {
                recent_set.push_back(leftIt->first);
                temp_result.push_back((ValueType) calc(op, (OpType) leftIt->second, (OpType) rightIt->second));
            }
            ++leftIt;
            ++rightIt;
        }
    }

    for (; leftIt != leftEnd;) {
        if (isActive(leftIt->first)) {
            recent_set.push_back(leftIt->first);
            temp_result.push_back((ValueType) calc(op, (OpType) leftIt->second, right));
        }
        ++leftIt;
    }

    for (; rightIt != rightEnd;) {
        if (isActive(rightIt->first)) {
            recent_set.push_back(rightIt->first);
            temp_result.push_back((ValueType) calc(op, left, (OpType) rightIt->second));
        }
        ++rightIt;
    }

#ifdef FORKED_BITSET

    for (int i = 0; i < forked_active_num; ++i) {
        forked_active_bitset[forked_active_set[i]] = false;
    }

#endif
}


#else

template<class OpType, class RetType>
static inline void process_T_calc_GoodVar(OpCodeType op, OpType left, OpType right, CalcType<OpType, RetType> calc,
                                   GoodVarIdType leftId, GoodVarIdType rightId) {
    GoodVarType &leftGoodVar = goodVarTable[leftId];
    GoodVarType &rightGoodVar = goodVarTable[rightId];

    // read good variable in id order

#ifdef USE_VECTOR
    iflikely(leftGoodVar.empty() && rightGoodVar.empty()) {
        return;
    }
#endif
    auto leftIt = leftGoodVar.begin();
    auto rightIt = rightGoodVar.begin();

#ifndef USE_VECTOR
    iflikely (leftIt == leftGoodVar.end() && rightIt == rightGoodVar.end()) {
        return;
    }
#endif

#ifdef FORKED_BITSET

    for (int i = 0; i < forked_active_num; ++i) {
        forked_active_bitset[forked_active_set[i]] = true;
    }

#endif

    for (; leftIt != leftGoodVar.end() && rightIt != rightGoodVar.end();) {
        if (leftIt->first < rightIt->first) {
            if (isActive(leftIt->first)) {
                recent_set.push_back(leftIt->first);
                temp_result.push_back((ValueType) calc(op, (OpType) leftIt->second, right));
            }
            ++leftIt;
        } else if (leftIt->first > rightIt->first) {
            if (isActive(rightIt->first)) {
                recent_set.push_back(rightIt->first);
                temp_result.push_back((ValueType) calc(op, left, (OpType) rightIt->second));
            }
            ++rightIt;
        } else {
            if (isActive(leftIt->first)) {
                recent_set.push_back(leftIt->first);
                temp_result.push_back((ValueType) calc(op, (OpType) leftIt->second, (OpType) rightIt->second));
            }
            ++leftIt;
            ++rightIt;
        }
    }

    for (; leftIt != leftGoodVar.end();) {
        if (isActive(leftIt->first)) {
            recent_set.push_back(leftIt->first);
            temp_result.push_back((ValueType) calc(op, (OpType) leftIt->second, right));
        }
        ++leftIt;
    }

    for (; rightIt != rightGoodVar.end();) {
        if (isActive(rightIt->first)) {
            recent_set.push_back(rightIt->first);
            temp_result.push_back((ValueType) calc(op, left, (OpType) rightIt->second));
        }
        ++rightIt;
    }

#ifdef FORKED_BITSET

    for (int i = 0; i < forked_active_num; ++i) {
        forked_active_bitset[forked_active_set[i]] = false;
    }

#endif
}

#endif

template<class OpType, class RetType>
static inline OpType process_T_calc_Mutation(OpCodeType op, OpType left, OpType right, CalcType<OpType, RetType> calc,
                                             MutationIdType mutId) {
    if (mutId == 0) {
        return calc(op, left, right);
    }

    Mutation *m = ALLMUTS[mutId];

    switch (m->type) {
        case LVR:
            if (m->op_0 == 0) {
                return calc(op, (OpType) m->op_2, right);
            } else if (m->op_0 == 1) {
                return calc(op, left, (OpType) m->op_2);
            } else {
                assert(false);
            }

        case UOI: {
            if (m->op_1 == 0) {
                OpType u_left;
                if (m->op_2 == 0) {
                    u_left = left + 1;
                } else if (m->op_2 == 1) {
                    u_left = left - 1;
                } else if (m->op_2 == 2) {
                    u_left = -left;
                } else {
                    assert(false);
                }
                return calc(op, u_left, right);
            } else if (m->op_1 == 1) {
                long u_right;
                if (m->op_2 == 0) {
                    u_right = right + 1;
                } else if (m->op_2 == 1) {
                    u_right = right - 1;
                } else if (m->op_2 == 2) {
                    u_right = -right;
                } else {
                    assert(false);
                }
                return calc(op, left, u_right);
            } else {
                assert(false);
            }
        }

        case ROV:
            return calc(op, right, left);

        case ABV:
            if (m->op_0 == 0) {
                return calc(op, abs(left), right);
            } else if (m->op_0 == 1) {
                return calc(op, left, abs(right));
            } else {
                assert(false);
            }

        case AOR:
        case LOR:
        case COR:
        case SOR:
            return calc(m->op_0, left, right);

        case ROR:
            return calc((int32_t) m->op_2, left, right);

        default:
            assert(false);
    }
}


static inline void filter_variant(MutationIdType from, MutationIdType to) {
    recent_set.clear();
    temp_result.clear();

    /*
    recent_set.push_back(0);

    if(MUTATION_ID == 0)
    {
        for (MutationIdType i = from; i < to; ++i)
        {
            recent_set.push_back(i);
        }
    }
     */

    if (MUTATION_ID == 0) {
        recent_set.push_back(0);
        for (MutationIdType i = from; i <= to; ++i) {
            if (default_active_set[i] == 1) {
                recent_set.push_back(i);
            }
        }
    } else {
        for (int i = 0; i < forked_active_num; ++i) {
            //fprintf(stderr, "act filter: %d\n", forked_active_num);
            if (forked_active_set[i] >= from && forked_active_set[i] <= to) {
                recent_set.push_back(forked_active_set[i]);
            }
            //fprintf(stderr, "recent filter: %ld\n", recent_set.size());
        }
        if (recent_set.empty()) {
            recent_set.push_back(0);
        }
    }

}


static inline ValueType fork_eqclass() {
    map<ValueType, vector<MutationIdType>> eqclasses;

    for (vector<MutationIdType>::size_type i = 1; i != recent_set.size(); ++i) {
        if (temp_result[i] != temp_result[0]) {
            eqclasses[temp_result[i]].push_back(recent_set[i]);
        }
    }

    fprintf(stderr, "Fork eqclass\n");
    fprintf(stderr, "%d\n", MUTATION_ID);
    for (int i = 0; i != recent_set.size(); ++i) {
        fprintf(stderr, "%d, ", recent_set[i]);
    }
    fprintf(stderr, "\n");

    for (auto it = eqclasses.begin(); it != eqclasses.end(); ++it) {
        int32_t isChild = goodvar_fork(it->second.front());

        if (isChild) {
            forked_active_num = 0;
            for (MutationIdType id : it->second) {
                forked_active_set[forked_active_num] = id;
                ++forked_active_num;
            }
            return it->first;
        } else {
            for (MutationIdType id : it->second) {
                default_active_set[id] = 0;
            }
        }
    }

    return temp_result[0];
}

#ifdef DEBUG_OUTPUT
static int good = 0;
static int bad = 0;

static int in = 0;
static int gvt = 0;
static int r = 0;
#endif

template<class OpType, class RetType>
static inline RetType process_T_arith_GoodVar
        (MutationIdType from, MutationIdType to, OpType left, OpType right,
         GoodVarIdType retId, GoodVarIdType leftId, GoodVarIdType rightId, OpCodeType op,
         CalcType<OpType, RetType> calc) {
    // TODO: We only consider order1 mutations, so mutations in following two parts won't intersect

    // calc mutation results
    filter_variant(from, to);
    for (MutationIdType mutId : recent_set) {
        temp_result.push_back((ValueType) process_T_calc_Mutation(op, left, right, calc, mutId));
    }

    //if (MUTATION_ID == 0)
    //    fprintf(stderr, "MAIN\n");
    //fprintf(stderr, "from: %d, to: %d, leftid: %d, rightid: %d, retid: %d\n", from, to, leftId, rightId, retId);

    // read good variables and calc results


#ifdef USE_VECTOR_POOL_WITH_ARRAY
    ifunlikely (goodVarTable.endPos != 0) {
        GoodVarType* lptr = nullptr;
        if (leftId != -1)
            lptr = goodVarTable.goodVarArray[leftId];
        GoodVarType* rptr = nullptr;
        if (rightId != -1)
            rptr = goodVarTable.goodVarArray[rightId];
        fprintf(stderr, "endpos: %d, %d, %d\n", goodVarTable.endPos, leftId, rightId);
        if (lptr == nullptr) {
            fprintf(stderr, "lempty\n");
            if (rptr == nullptr) {
                fprintf(stderr, "rempty\n");

            } else {
                fprintf(stderr, "rfull\n");
                process_T_calc_GoodVar_Right(op, left, calc, rptr->cbegin(), rptr->cend());
            }
        } else {
            fprintf(stderr, "lfull\n");
            if (rptr == nullptr) {
                process_T_calc_GoodVar_Left(op, right, calc, lptr->cbegin(), lptr->cend());
            } else {
                fprintf(stderr, "rfull\n");
                fprintf(stderr, "size: %ld\n", lptr->size());
                process_T_calc_GoodVar(op, left, right, calc, lptr->cbegin(), lptr->cend(),
                                       rptr->begin(), rptr->cend());
            }
        }
    }
#elif defined SPLIT
    if (!goodVarTable.empty()) {
        auto lIt = goodVarTable.find(leftId);
        auto rIt = goodVarTable.find(rightId);
        if (lIt == goodVarTable.end()) {
            if (rIt == goodVarTable.end()) {

            } else {
                process_T_calc_GoodVar_Right(op, left, calc, rIt->second.cbegin(), rIt->second.cend());
            }
        } else {
            if (rIt == goodVarTable.end()) {
                process_T_calc_GoodVar_Left(op, right, calc, lIt->second.cbegin(), lIt->second.cend());
            } else {
                process_T_calc_GoodVar(op, left, right, calc, lIt->second.cbegin(), lIt->second.cend(),
                                       rIt->second.cbegin(), rIt->second.cend());
            }
        }
    }
#else
        if (!goodVarTable.empty()) {
            process_T_calc_GoodVar(op, left, right, calc, leftId, rightId);
        }
#endif

    iflikely (recent_set.size() == 1 && recent_set[0] == 0) {
        return (RetType) temp_result[0];
    }
#ifdef DEBUG_OUTPUT
    ++in;
#endif
    if (retId == BADVAR) {
#ifdef DEBUG_OUTPUT
        ++gvt;
        fprintf(stderr, "BAD: %d, %d, %d\n", in, gvt, r);
#endif
        return (RetType) fork_eqclass();
    } else {
#ifdef DEBUG_OUTPUT
        ++r;
        fprintf(stderr, "IN: %d, %d, %d\n", in, gvt, r);
#endif

#ifdef USE_VECTOR_POOL_WITH_ARRAY
        size_t retGoodVarId = gvpool.get();
        GoodVarType *retGoodVar = gvpool[retGoodVarId];
        for (vector<MutationIdType>::size_type i = 1; i != recent_set.size(); ++i) {
            if (temp_result[i] != temp_result[0]) {
                retGoodVar->push_back(std::make_pair(recent_set[i], temp_result[i]));
            }
        }

        if (!retGoodVar->empty()) {
            goodVarTable.goodVarArray[retId] = retGoodVar;
            goodVarTable.needRetArray[goodVarTable.endPos++] = retGoodVarId;
        } else {
            gvpool.unget(retGoodVarId);
        }

#else

#ifdef SPLIT
        GoodVarType retGoodVar;

#else

        GoodVarType &retGoodVar = goodVarTable[retId];
#endif
        // TODO: Here only consider order1 mutation, skip the first mutation which is actually running on CPU
        for (vector<MutationIdType>::size_type i = 1; i != recent_set.size(); ++i) {
            if (temp_result[i] != temp_result[0]) {
#ifdef USE_VECTOR
                retGoodVar.push_back(std::make_pair(recent_set[i], temp_result[i]));
#else
                retGoodVar[recent_set[i]] = temp_result[i];
#endif
            }
        }

#ifdef SPLIT
        if (!retGoodVar.empty()) {
            std::sort(retGoodVar.begin(), retGoodVar.end(), [](const std::pair<MutationIdType, ValueType> a,
                                                               const std::pair<MutationIdType, ValueType> b) {
                return a.first < b.first;
            });
            retGoodVar.erase(
                    std::unique(retGoodVar.begin(), retGoodVar.end(), [](const std::pair<MutationIdType, ValueType> a,
                                                                         const std::pair<MutationIdType, ValueType> b) {
                        return a.first < b.first;
                    }), retGoodVar.end());
            assert(std::is_sorted(retGoodVar.begin(), retGoodVar.end(), [](const std::pair<MutationIdType, ValueType> a,
                                                                           const std::pair<MutationIdType, ValueType> b) {
                return a.first < b.first;
            }));
            goodVarTable[retId] = std::move(retGoodVar);
        }
#endif
#endif

#ifdef DEBUG_OUTPUT

        /*
        if (retGoodVar->size() != 0)
            good++;
        else
            bad++;
        fprintf(stderr, "id: %d, Good: %d, bad: %d\n", MUTATION_ID, good, bad);
        */
        // fprintf(stderr, "recent size: %ld, ret size: %ld\n", recent_set.size(), retGoodVar->size());

#endif

        return (RetType) temp_result[0];
    }
}


int32_t __accmut__process_i32_arith_GoodVar
        (RegMutInfo *rmi, int32_t from, int32_t to, int32_t left, int32_t right,
         int32_t retId, int32_t leftId, int32_t rightId, int32_t op) {
    __accmut__register(rmi);
#ifdef DEBUG_OUTPUT
    fprintf(stderr, "%d, %d, %d\n", MUTATION_ID, from, to);
#endif
    from += rmi->offset;
    to += rmi->offset;

    return process_T_arith_GoodVar(from, to, left, right, retId, leftId, rightId, op, __accmut__cal_i32_arith);
}


int64_t __accmut__process_i64_arith_GoodVar
        (RegMutInfo *rmi, int32_t from, int32_t to, int64_t left, int64_t right,
         int32_t retId, int32_t leftId, int32_t rightId, int32_t op) {
    __accmut__register(rmi);
#ifdef DEBUG_OUTPUT
    fprintf(stderr, "%d, %d, %d\n", MUTATION_ID, from, to);
#endif
    from += rmi->offset;
    to += rmi->offset;
    return process_T_arith_GoodVar(from, to, left, right, retId, leftId, rightId, op, __accmut__cal_i64_arith);
}


int32_t __accmut__process_i32_cmp_GoodVar
        (RegMutInfo *rmi, int32_t from, int32_t to, int32_t left, int32_t right,
         int32_t retId, int32_t leftId, int32_t rightId, int32_t pred) {
    assert(retId == BADVAR);
#ifdef DEBUG_OUTPUT
    fprintf(stderr, "%d, %d, %d\n", MUTATION_ID, from, to);
#endif
    __accmut__register(rmi);
    from += rmi->offset;
    to += rmi->offset;
    return process_T_arith_GoodVar(from, to, left, right, retId, leftId, rightId, pred, __accmut__cal_i32_bool);
}


int32_t __accmut__process_i64_cmp_GoodVar
        (RegMutInfo *rmi, int32_t from, int32_t to, int64_t left, int64_t right,
         int32_t retId, int32_t leftId, int32_t rightId, int32_t pred) {
    assert(retId == BADVAR);
#ifdef DEBUG_OUTPUT
    fprintf(stderr, "%d, %d, %d\n", MUTATION_ID, from, to);
#endif
    __accmut__register(rmi);
    from += rmi->offset;
    to += rmi->offset;
    return process_T_arith_GoodVar(from, to, left, right, retId, leftId, rightId, pred, __accmut__cal_i64_bool);
}
