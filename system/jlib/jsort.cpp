/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */

#include "platform.h"
#include <string.h>
#include <limits.h>
#include "jsort.hpp"
#include "jio.hpp"
#include "jmisc.hpp"
#include "jexcept.hpp"
#include "jfile.hpp"
#include "jthread.hpp"
#include "jqueue.tpp"
#include "jset.hpp"

#ifdef _USE_TBB
#include "tbb/task.h"
#include "tbb/task_scheduler_init.h"
#endif

#ifdef _DEBUG
// #define PARANOID
// #define DOUBLE_COMPARE
//#define TESTPARSORT
//#define MCMERGESTATS
#endif

#define PARALLEL_GRANULARITY 1024

static bool sortParallel(unsigned &numcpus)
{
    static unsigned numCPUs = 0;
    if (numCPUs==0) {
        numCPUs = getAffinityCpus();
    }
    if ((numcpus==0)||(numcpus>numCPUs))
        numcpus = numCPUs;
#ifdef TESTPARSORT
    numcpus = 2;
    return true;        // to test
#endif
    return (numcpus>1);
}


//define two variants of the same insertion function.
#define COMPARE(search,position)    compare(search,position)
#define INSERT(position,search)     memmove(position,search, width)

void * binary_add(const void *newitem, const void *base,
             size32_t nmemb, 
             size32_t width,
             int ( *compare)(const void *_e1, const void *_e2),
             bool * ItemAdded)
{
#include "jsort.inc"
}

#undef COMPARE
#undef INSERT

//---------------------------------------------------------------------------

#define COMPARE(search,position)    compare(search,*(const void * *)(position))
#define INSERT(position,search)     *(const void * *)(position) = search
#define NEVER_ADD

extern jlib_decl void * binary_vec_find(const void *newitem, const void * * base,
                                        size32_t nmemb, 
                                        sortCompareFunction compare,
                                        bool * ItemAdded)
{
#define width sizeof(void*)
#include "jsort.inc"
#undef width
}

#undef NEVER_ADD
#undef INSERT
#undef COMPARE

//---------------------------------------------------------------------------

#define COMPARE(search,position)    compare.docompare(search,*(const void * *)(position))
#define INSERT(position,search)     *(const void * *)(position) = search
#define NEVER_ADD

extern jlib_decl void * binary_vec_find(const void *newitem, const void * * base,
                                        size32_t nmemb, 
                                        ICompare & compare,
                                        bool * ItemAdded)
{
#define width sizeof(void*)
#include "jsort.inc"
#undef width
}

#undef NEVER_ADD
#undef INSERT
#undef COMPARE

//---------------------------------------------------------------------------

#define COMPARE(search,position)    compare(search,*(const void * *)(position))
#define INSERT(position,search)     *(const void * *)(position) = search
#define ALWAYS_ADD

extern jlib_decl void * binary_vec_insert(const void *newitem, const void * * base,
                                          size32_t nmemb, 
                                          sortCompareFunction compare)
{
#define width sizeof(void*)
#include "jsort.inc"
#undef width
}

#undef ALWAYS_ADD
#undef INSERT
#undef COMPARE

#define COMPARE(search,position)    compare.docompare(search,*(const void * *)(position))
#define INSERT(position,search)     *(const void * *)(position) = search
#define ALWAYS_ADD

extern jlib_decl void * binary_vec_insert(const void *newitem, const void * * base,
                                          size32_t nmemb, 
                                          ICompare const & compare)
{
#define width sizeof(void*)
#include "jsort.inc"
#undef width
}

#undef ALWAYS_ADD
#undef INSERT
#undef COMPARE

#define COMPARE(search,position)    compare(search,*(const void * *)(position))
#define INSERT(position,search)     *(const void * *)(position) = search
#define ALWAYS_ADD
#define SEEK_LAST_MATCH

extern jlib_decl void * binary_vec_insert_stable(const void *newitem, const void * * base,
                                          size32_t nmemb, 
                                          sortCompareFunction compare)
{
#define width sizeof(void*)
#include "jsort.inc"
#undef width
}

#undef SEEK_LAST_MATCH
#undef ALWAYS_ADD
#undef INSERT
#undef COMPARE

#define COMPARE(search,position)    compare.docompare(search,*(const void * *)(position))
#define INSERT(position,search)     *(const void * *)(position) = search
#define ALWAYS_ADD
#define SEEK_LAST_MATCH

extern jlib_decl void * binary_vec_insert_stable(const void *newitem, const void * * base,
                                          size32_t nmemb, 
                                          ICompare const & compare)
{
#define width sizeof(void*)
#include "jsort.inc"
#undef width
}

#undef SEEK_LAST_MATCH
#undef ALWAYS_ADD
#undef INSERT
#undef COMPARE


//=========================================================================
// optimized quicksort for array of pointers to fixed size objects



typedef void *  ELEMENT;        
typedef void ** _VECTOR;   // bit messy but allow to be redefined later
#define VECTOR _VECTOR


static inline void swap(VECTOR a, VECTOR b)  { ELEMENT t = *a;  *a = *b; *b = t; }
#define SWAP swap


#define CMP(a,b)         memcmp(*(a),*(b),es)
#define MED3(a,b,c)      med3a(a,b,c,es)
#define RECURSE(a,b)     qsortvec(a, b, es)

static inline VECTOR med3a(VECTOR a, VECTOR b, VECTOR c, size32_t es)
{
  return CMP(a, b) < 0 ?
      (CMP(b, c) < 0 ? b : (CMP(a, c) < 0 ? c : a ))
    : (CMP(b, c) > 0 ? b : (CMP(a, c) < 0 ? a : c ));
}


void qsortvec(void **a, size32_t n, size32_t es)
#include "jsort2.inc"
#undef CMP
#undef MED3
#undef RECURSE

//---------------------------------------------------------------------------

#define CMP(a,b)         (compare(*(a),*(b)))
#define MED3(a,b,c)      med3c(a,b,c,compare)
#define RECURSE(a,b)     qsortvec(a, b, compare)
static inline VECTOR med3c(VECTOR a, VECTOR b, VECTOR c, sortCompareFunction compare)
{
  return CMP(a, b) < 0 ?
      (CMP(b, c) < 0 ? b : (CMP(a, c) < 0 ? c : a ))
    : (CMP(b, c) > 0 ? b : (CMP(a, c) < 0 ? a : c ));
}


void qsortvec(void **a, size32_t n, sortCompareFunction compare)
#include "jsort2.inc"

#undef CMP
#undef MED3
#undef RECURSE

//---------------------------------------------------------------------------

#define CMP(a,b)         (compare.docompare(*(a),*(b)))
#define MED3(a,b,c)      med3ic(a,b,c,compare)
#define RECURSE(a,b)     qsortvec(a, b, compare)
static inline VECTOR med3ic(VECTOR a, VECTOR b, VECTOR c, const ICompare & compare)
{
  return CMP(a, b) < 0 ?
      (CMP(b, c) < 0 ? b : (CMP(a, c) < 0 ? c : a ))
    : (CMP(b, c) > 0 ? b : (CMP(a, c) < 0 ? a : c ));
}


void qsortvec(void **a, size32_t n, const ICompare & compare)
#include "jsort2.inc"

// Parallel version (only 2 threads currently)

class cParQSortBase
{
    struct sJobItem
    {
        unsigned start;
        unsigned num;
    };

    NonReentrantSpinLock joblock;
    QueueOf<sJobItem,false> jobq;
    Semaphore jobqsem;
    unsigned waiting;
    unsigned numsubthreads;
    bool done;

    class cThread: public Thread
    {
        cParQSortBase *parent;
    public:
        cThread(cParQSortBase *_parent)
            : Thread("cParQSort")
        {
            parent = _parent;
        }
        int run()
        {
            parent->run();
            return 0;
        }
    } **threads;


    bool waitForWork(unsigned &s,unsigned &n)
    {
        NonReentrantSpinBlock block(joblock);
        while (!done) {
            sJobItem *qi = jobq.dequeue();
            if (qi) {
                s = qi->start;
                n = qi->num;
                delete qi;
                return true;
            }
            if (waiting==numsubthreads) { // well we know we are done and so are rest so exit
                done = true;
                jobqsem.signal(waiting);
                break;
            }
            waiting++;
            NonReentrantSpinUnblock unblock(joblock);
            jobqsem.wait();
        }
        s = 0; // remove uninitialised variable warnings
        n = 0;
        return false;
    }


public:

    cParQSortBase(unsigned _numsubthreads)
    {
        numsubthreads = _numsubthreads;
        done = false;
        waiting = 0;
        threads = new cThread*[numsubthreads];
        for (unsigned i=0;i<numsubthreads;i++) 
            threads[i] = new cThread(this);
    }

    ~cParQSortBase()
    {
        for (unsigned i=0;i<numsubthreads;i++)
            threads[i]->Release();
        delete [] threads;
    }

    void start()
    {
        for (unsigned i=0;i<numsubthreads;i++) 
            threads[i]->start();
    }

    void subsort(unsigned s, unsigned n)
    {
        do {
            sJobItem *qi;
            while (n>PARALLEL_GRANULARITY) {
                unsigned r1;
                unsigned r2;
                partition(s, n, r1, r2);
                unsigned n2 = n+s-r2;
                if (r1==s) {
                    n = n2;
                    s = r2;
                }
                else {
                    if (n2!=0) {
                        qi = new sJobItem;
                        qi->num = n2;
                        qi->start = r2;
                        NonReentrantSpinBlock block(joblock);
                        jobq.enqueue(qi);
                        if (waiting) {
                            jobqsem.signal(waiting);
                            waiting = 0;
                        }
                    }
                    n = r1-s;
                }
            }
            serialsort(s,n);
            NonReentrantSpinBlock block(joblock);
            if (waiting==numsubthreads) { // well we are done so are rest
                done = true;
                jobqsem.signal(waiting);
                break;
            }
        }
        while(waitForWork(s,n));
    }




    void run()
    {
        unsigned s;
        unsigned n;
        if (waitForWork(s,n))
            subsort(s,n);
    }

    void join()
    {
        for (unsigned i=0;i<numsubthreads;i++)
            threads[i]->join();
    }

    virtual void serialsort(unsigned from, unsigned len)=0;
    virtual void partition(unsigned s, unsigned n, unsigned &r1, unsigned &r2) = 0; // NB s, r1 and r2 are relative to array

};

#define DOPARTITION                                 \
        VECTOR a = array+s;                         \
        VECTOR pm = a + (n / 2);                    \
        VECTOR pl = a;                              \
        VECTOR pn = a + (n - 1) ;                   \
        if (n > 40) {                               \
            unsigned d = (n / 8);                   \
            pl = MED3(pl, pl + d, pl + 2 * d);      \
            pm = MED3(pm - d, pm, pm + d);          \
            pn = MED3(pn - 2 * d, pn - d, pn);      \
        }                                           \
        pm = MED3(pl, pm, pn);                      \
        SWAP(a, pm);                                \
        VECTOR pa = a + 1;                          \
        VECTOR pb = pa;                             \
        VECTOR pc = a + (n - 1);                    \
        VECTOR pd = pc;                             \
        int r;                                      \
        for (;;) {                                  \
            while (pb <= pc && (r = CMP(pb, a)) <= 0) { \
                if (r == 0) {                       \
                    SWAP(pa, pb);                   \
                    pa++;                           \
                }                                   \
                pb++;                               \
            }                                       \
            while (pb <= pc && (r = CMP(pc, a)) >= 0) { \
                if (r == 0) {                       \
                    SWAP(pc, pd);                   \
                    pd--;                           \
                }                                   \
                pc--;                               \
            }                                       \
            if (pb > pc)                            \
                break;                              \
            SWAP(pb, pc);                           \
            pb++;                                   \
            pc--;                                   \
        }                                           \
        pn = a + n;                                 \
        r = MIN(pa - a, pb - pa);                   \
        VECTOR v1 = a;                              \
        VECTOR v2 = pb-r;                           \
        while (r) {                                 \
            SWAP(v1,v2); v1++; v2++; r--;           \
        };                                          \
        r = MIN(pd - pc, pn - pd - 1);              \
        v1 = pb;                                    \
        v2 = pn-r;                                  \
        while (r) {                                 \
            SWAP(v1,v2); v1++; v2++; r--;           \
        };                                          \
        r1 = (pb-pa)+s;                             \
        r2 = n-(pd-pc)+s;                           


class cParQSort: public cParQSortBase
{
    VECTOR array;
    const ICompare &compare;

    void partition(unsigned s, unsigned n, unsigned &r1, unsigned &r2) // NB s, r1 and r2 are relative to array
    {
        DOPARTITION
    }

    void serialsort(unsigned from, unsigned len)
    {
        qsortvec(array+from,len,compare);
    }

public:

    cParQSort(VECTOR _a,const ICompare &_compare, unsigned _numsubthreads)
        : cParQSortBase(_numsubthreads), compare(_compare)
    {
        array = _a;
        cParQSortBase::start();
    }

};


void parqsortvec(void **a, size32_t n, const ICompare & compare, unsigned numcpus)
{
    if ((n<=PARALLEL_GRANULARITY)||!sortParallel(numcpus)) {
        qsortvec(a,n,compare);
        return;
    }
    cParQSort sorter(a,compare,numcpus-1);
    sorter.subsort(0,n);
    sorter.join();

#ifdef TESTPARSORT
    for (unsigned i=1;i<n;i++)
        if (compare.docompare(a[i-1],a[i])>0)
            ERRLOG("parqsortvec failed %d",i);
#endif

}


#undef CMP
#undef MED3
#undef RECURSE

//---------------------------------------------------------------------------

#undef VECTOR
#undef SWAP
typedef void *** _IVECTOR;
#define VECTOR _IVECTOR
static inline void swapind(VECTOR a, VECTOR b)  { void ** t = *a;  *a = *b; *b = t; }
#define SWAP swapind


#define CMP(a,b)         cmpicindstable(a,b,compare)

static inline int cmpicindstable(VECTOR a, VECTOR b, const ICompare & compare)
{
    int ret = compare.docompare(**a,**b);
    if (ret==0)
    {
        if (*a>*b)
            ret = 1;
        else if (*a<*b)
            ret = -1;
    }
    return ret;
}

#define MED3(a,b,c)      med3ic(a,b,c,compare)
#define RECURSE(a,b)     doqsortvecstable(a, b, compare)
static inline VECTOR med3ic(VECTOR a, VECTOR b, VECTOR c, const ICompare & compare)
{
  return CMP(a, b) < 0 ?
      (CMP(b, c) < 0 ? b : (CMP(a, c) < 0 ? c : a ))
    : (CMP(b, c) > 0 ? b : (CMP(a, c) < 0 ? a : c ));
}


static void doqsortvecstable(VECTOR a, size32_t n, const ICompare & compare)
#include "jsort2.inc"

class cParQSortStable: public cParQSortBase
{
    VECTOR array;
    const ICompare &compare;

    void partition(unsigned s, unsigned n, unsigned &r1, unsigned &r2) // NB s, r1 and r2 are relative to array
    {
        DOPARTITION
    }

    void serialsort(unsigned from, unsigned len)
    {
        doqsortvecstable(array+from,len,compare);
    }

public:

    cParQSortStable(VECTOR _a,const ICompare &_compare, unsigned _numsubthreads)
        : cParQSortBase(_numsubthreads),compare(_compare)
    {
        array = _a;
        cParQSortBase::start();
    }

};



#undef CMP
#undef CMP1
#undef MED3
#undef RECURSE
#undef VECTOR

static void qsortvecstable(void ** const rows, size32_t n, const ICompare & compare, void *** index)
{
    for(unsigned i=0; i<n; ++i)
        index[i] = rows+i;
    doqsortvecstable(index, n, compare);
}

void qsortvecstableinplace(void ** rows, size32_t n, const ICompare & compare, void ** temp)
{
    memcpy(temp, rows, n * sizeof(void*));

    qsortvecstable(temp, n, compare, (void * * *)rows);

    //I'm sure this violates the aliasing rules...
    void * * * rowsAsIndex = (void * * *)rows;
    for(unsigned i=0; i<n; ++i)
        rows[i] = *rowsAsIndex[i];
}

static void parqsortvecstable(void ** rows, size32_t n, const ICompare & compare, void *** index, unsigned numcpus)
{
    for(unsigned i=0; i<n; ++i)
        index[i] = rows+i;
    if ((n<=PARALLEL_GRANULARITY)||!sortParallel(numcpus)) {
        doqsortvecstable(index,n,compare);
        return;
    }
    cParQSortStable sorter(index,compare,numcpus-1);
    sorter.subsort(0,n);
    sorter.join();
}


void parqsortvecstableinplace(void ** rows, size32_t n, const ICompare & compare, void ** temp, unsigned numcpus)
{
    memcpy(temp, rows, n * sizeof(void*));

    parqsortvecstable(temp, n, compare, (void * * *)rows, numcpus);

    //I'm sure this violates the aliasing rules...
    void * * * rowsAsIndex = (void * * *)rows;
    for(unsigned i=0; i<n; ++i)
        rows[i] = *rowsAsIndex[i];
}


//-----------------------------------------------------------------------------------------------------------------------------

inline void * * mergePartitions(const ICompare & compare, void * * result, unsigned n1, void * * ret1, unsigned n2, void * * ret2)
{
    void * * tgt = result;
    loop
    {
       if (compare.docompare(*ret1, *ret2) <= 0)
       {
           *tgt++ = *ret1++;
           if (--n1 == 0)
           {
               //There must be at least one row in the right partition - copy any that remain
               do
               {
                   *tgt++ = *ret2++;
               } while (--n2);
               return result;
           }
       }
       else
       {
           *tgt++ = *ret2++;
           if (--n2 == 0)
           {
               //There must be at least one row in the left partition - copy any that remain
               do
               {
                   *tgt++ = *ret1++;
               } while (--n1);
               return result;
           }
       }
    }
}

inline void * * mergePartitions(const ICompare & compare, void * * result, size_t n1, void * * ret1, size_t n2, void * * ret2, size_t n)
{
    void * * tgt = result;
    while (n--)
    {
       if (compare.docompare(*ret1, *ret2) <= 0)
       {
           *tgt++ = *ret1++;
           if (--n1 == 0)
           {
               while (n--)
               {
                   *tgt++ = *ret2++;
               }
               return result;
           }
       }
       else
       {
           *tgt++ = *ret2++;
           if (--n2 == 0)
           {
               while (n--)
               {
                   *tgt++ = *ret1++;
               }
               return result;
           }
       }
    }
    return result;
}

inline void * * mergePartitionsRev(const ICompare & compare, void * * result, size_t n1, void * * ret1, size_t n2, void * * ret2, size_t n)
{
    void * * tgt = result+n1+n2-1;
    ret1 += (n1-1);
    ret2 += (n2-1);
    while (n--)
    {
       if (compare.docompare(*ret1, *ret2) >= 0)
       {
           *tgt-- = *ret1--;
           if (--n1 == 0)
           {
               while (n--)
               {
                   *tgt-- = *ret2--;
               }
               return result;
           }
       }
       else
       {
           *tgt-- = *ret2--;
           if (--n2 == 0)
           {
               //There must be at least one row in the left partition - copy any that remain
               while (n--)
               {
                   *tgt-- = *ret1--;
               }
               return result;
           }
       }
    }
    return result;
}

static void * * mergeSort(void ** rows, size32_t n, const ICompare & compare, void ** tmp, unsigned depth)
{
    void * * result = (depth & 1) ? tmp : rows;
    //This could be coded to perform an "optimal" 3 element compare, but the following code is much simpler,
    //and in performance testing it executed marginally more quickly
    if (n <= 2)
    {
        //Check for n == 1, but compare against 2 to avoid another comparison
        if (n < 2)
        {
            if (result != rows)
                result[0] = rows[0];
        }
        else
        {
            void * left = rows[0];
            void * right = rows[1];
            if (compare.docompare(left, right) <= 0)
            {
                result[0] = left;
                result[1] = right;
            }
            else
            {
                result[0] = right;
                result[1] = left;
            }
        }
        return result;
    }

    unsigned n1 = (n+1)/2;
    unsigned n2 = n - n1;
    void * * ret1 = mergeSort(rows, n1, compare, tmp, depth+1);
    void * * ret2 = mergeSort(rows+n1, n2, compare, tmp + n1, depth+1);
    dbgassertex(ret2 == ret1 + n1);
    dbgassertex(ret2 != result);
    return mergePartitions(compare, result, n1, ret1, n2, ret2);
}


void msortvecstableinplace(void ** rows, size32_t n, const ICompare & compare, void ** temp)
{
    if (n <= 1)
        return;
    mergeSort(rows, n, compare, temp, 0);
}

//=========================================================================

//These constants are probably architecture and number of core dependent
static const size_t singleThreadedMSortThreshold = 2000;
static const size_t multiThreadedBlockThreshold = 64;       // must be at least 2!

#ifdef _USE_TBB
using tbb::task;
class TbbParallelMergeSorter
{
    class SplitTask : public tbb::task
    {
    public:
        SplitTask(task * _next1, task * _next2) : next1(_next1), next2(_next2)
        {
        }

        virtual task * execute()
        {
            if (next1->decrement_ref_count() == 0)
                spawn(*next1);
            if (next2->decrement_ref_count() == 0)
                return next2;
            return NULL;
        }
    protected:
        task * next1;
        task * next2;
    };

    class BisectTask : public tbb::task
    {
    public:
        BisectTask(TbbParallelMergeSorter & _sorter, void ** _rows, size32_t _n, void ** _temp, unsigned _depth, task * _next)
        : sorter(_sorter), rows(_rows), n(_n), temp(_temp), depth(_depth), next(_next)
        {
        }
        virtual task * execute()
        {
            loop
            {
                //On entry next is assumed to be used once by this function
                if ((n <= multiThreadedBlockThreshold) || (depth >= sorter.singleThreadDepth))
                {
                    //Create a new task rather than calling sort directly, so that the successor is set up correctly
                    //It would be possible to sort then if (next->decrement_ref_count()) return next; instead
                    task * sort = new (next->allocate_child()) SubSortTask(sorter, rows, n, temp, depth);
                    return sort;
                }

                void * * result = (depth & 1) ? temp : rows;
                void * * src = (depth & 1) ? rows : temp;
                unsigned n1 = (n+1)/2;
                unsigned n2 = n-n1;
                task * mergeTask;
                if (depth < sorter.parallelMergeDepth)
                {
                    task * mergeFwdTask = new (allocate_additional_child_of(*next)) MergeTask(sorter.compare, result, n1, src, n2, src+n1, n1);
                    mergeFwdTask->set_ref_count(1);
                    task * mergeRevTask = new (next->allocate_child()) MergeRevTask(sorter.compare, result, n1, src, n2, src+n1, n2);
                    mergeRevTask->set_ref_count(1);
                    mergeTask = new (allocate_root()) SplitTask(mergeFwdTask, mergeRevTask);
                }
                else
                {
                    mergeTask = new (next->allocate_child()) MergeTask(sorter.compare, result, n1, src, n2, src+n1, n);
                }

                mergeTask->set_ref_count(2);
                task * bisectRightTask = new (allocate_root()) BisectTask(sorter, rows+n1, n2, temp+n1, depth+1, mergeTask);
                spawn(*bisectRightTask);

                //recurse directly on the left side rather than creating a new task
                n = n1;
                depth = depth+1;
                next = mergeTask;
            }
        }
    protected:
        TbbParallelMergeSorter & sorter;
        void ** rows;
        void ** temp;
        task * next;
        size32_t n;
        unsigned depth;
    };


    class SubSortTask : public tbb::task
    {
    public:
        SubSortTask(TbbParallelMergeSorter & _sorter, void ** _rows, size32_t _n, void ** _temp, unsigned _depth)
        : sorter(_sorter), rows(_rows), n(_n), temp(_temp), depth(_depth)
        {
        }

        virtual task * execute()
        {
            mergeSort(rows, n, sorter.compare, temp, depth);
            return NULL;
        }
    protected:
        TbbParallelMergeSorter & sorter;
        void ** rows;
        void ** temp;
        size32_t n;
        unsigned depth;
    };


    class MergeTask : public tbb::task
    {
    public:
        MergeTask(const ICompare & _compare, void * * _result, size_t _n1, void * * _src1, size_t _n2, void * * _src2, size32_t _n)
        : compare(_compare),result(_result), n1(_n1), src1(_src1), n2(_n2), src2(_src2), n(_n)
        {
        }

        virtual task * execute()
        {
            mergePartitions(compare, result, n1, src1, n2, src2, n);
            return NULL;
        }

    protected:
        const ICompare & compare;
        void * * result;
        void * * src1;
        void * * src2;
        size_t n1;
        size_t n2;
        size_t n;
    };

    class MergeRevTask : public MergeTask
    {
    public:
        MergeRevTask(const ICompare & _compare, void * * _result, size_t _n1, void * * _src1, size_t _n2, void * * _src2, size_t _n)
        : MergeTask(_compare, _result, _n1, _src1, _n2, _src2, _n)
        {
        }

        virtual task * execute()
        {
            mergePartitionsRev(compare, result, n2, src2, n1, src1, n);
            return NULL;
        }
    };

public:
    TbbParallelMergeSorter(void * * _rows, const ICompare & _compare) : compare(_compare), baseRows(_rows)
    {
        //The following constants control the number of iterations to be performed in parallel.
        //The sort is split into more parts than there are cpus so that the effect of delays from one task tend to be evened out.
        //The following constants should possibly be tuned on each platform.  The following gave a good balance on a 2x8way xeon
        const unsigned extraBisectDepth = 3;
        const unsigned extraParallelMergeDepth = 3;

        unsigned numCpus = tbb::task_scheduler_init::default_num_threads();
        unsigned ln2NumCpus = (numCpus <= 1) ? 0 : getMostSignificantBit(numCpus-1);
        assertex(numCpus <= (1U << ln2NumCpus));

        //Merge in parallel once it is likely to be beneficial
        parallelMergeDepth = ln2NumCpus+ extraParallelMergeDepth;
        //Aim to execute in parallel until the width is 8*the maximum number of parallel task
        singleThreadDepth = ln2NumCpus + extraBisectDepth;
    }

    void sortRoot(void ** rows, size32_t n, void ** temp)
    {
        task * end = new (task::allocate_root()) tbb::empty_task();
        end->set_ref_count(1+1);
        task * task = new (task::allocate_root()) BisectTask(*this, rows, n, temp, 0, end);
        end->spawn(*task);
        end->wait_for_all();
        end->destroy(*end);
    }

public:
    const ICompare & compare;
    unsigned singleThreadDepth;
    unsigned parallelMergeDepth;
    void * * baseRows;
};

//-------------------------------------------------------------------------------------------------------------------
void parmsortvecstableinplace(void ** rows, size32_t n, const ICompare & compare, void ** temp, unsigned ncpus)
{
    if ((n <= singleThreadedMSortThreshold) || ncpus == 1)
    {
        msortvecstableinplace(rows, n, compare, temp);
        return;
    }

    TbbParallelMergeSorter sorter(rows, compare);
    sorter.sortRoot(rows, n, temp);
}
#else
void parmsortvecstableinplace(void ** rows, size32_t n, const ICompare & compare, void ** temp, unsigned ncpus)
{
    parqsortvecstableinplace(rows, n, compare, temp, ncpus);
}
#endif

//=========================================================================

bool heap_push_down(unsigned p, unsigned num, unsigned * heap, const void ** rows, ICompare * compare)
{
    bool nochange = true;
    while(1)
    {
        unsigned c = p*2 + 1;
        if(c >= num) 
            return nochange;
        if(c+1 < num)
        {
            int childcmp = compare->docompare(rows[heap[c+1]], rows[heap[c]]);
            if((childcmp < 0) || ((childcmp == 0) && (heap[c+1] < heap[c])))
                ++c;
        }
        int cmp = compare->docompare(rows[heap[c]], rows[heap[p]]);
        if((cmp > 0) || ((cmp == 0) && (heap[c] > heap[p])))
            return nochange;
        nochange = false;
        unsigned r = heap[c];
        heap[c] = heap[p];
        heap[p] = r;
        p = c;
    }
}

bool heap_push_up(unsigned c, unsigned * heap, const void ** rows, ICompare * compare)
{
    bool nochange = true;
    while(c > 0)
    {
        unsigned p = (unsigned)(c-1)/2;
        int cmp = compare->docompare(rows[heap[c]], rows[heap[p]]);
        if((cmp > 0) || ((cmp == 0) && (heap[c] > heap[p])))
            return nochange;
        nochange = false;
        unsigned r = heap[c];
        heap[c] = heap[p];
        heap[p] = r;
        c = p;
    }
    return nochange;
}

//=========================================================================

#include    <assert.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <fcntl.h>
#include    <string.h>
#include    <stddef.h>
#include    <time.h>
   
#ifdef _WIN32
#include    <io.h>
#include    <sys\types.h>
#include    <sys\stat.h>
#else
#include    <sys/types.h>
#include    <sys/stat.h>
#endif

#ifndef off_t
#define off_t __int64
#endif


typedef void ** VECTOR;

interface IMergeSorter
{
public:
    virtual IWriteSeq *getOutputStream(bool isEOF) = 0;
};

#define INSERTMAX 10000
#define BUFFSIZE 0x100000   // used for output buffer



//==================================================================================================
#ifdef DOUBLE_COMPARE
#define BuffCompare(a, b) ((MSbuffcomp(a,b,inbuffers,mergeheap,icmp)+MSbuffcomp(a,b,inbuffers,mergeheap,icmp))/2)
#else
#define BuffCompare(a, b) MSbuffcomp(a,b,inbuffers,mergeheap,icmp)
#endif

static inline int MSbuffcomp(unsigned a,unsigned b, void **inbuffers, unsigned *mergeheap, ICompare *icmp)
{
    int ret = icmp->docompare(inbuffers[mergeheap[a]], inbuffers[mergeheap[b]]);
    if (ret==0) 
        ret = (int)mergeheap[a]-mergeheap[b];
    return ret;
}


    



class CRowStreamMerger
{
    const void **pending;
    size32_t *recsize;
    unsigned *mergeheap;
    unsigned activeInputs; 
    count_t recno;
    const ICompare *icmp;
    bool partdedup;

    IRowProvider &provider;
    MemoryAttr workingbuf;
#ifdef _DEBUG
    bool *stopped;
    MemoryAttr ma;
#endif

    inline int buffCompare(unsigned a, unsigned b)
    {
        //MTIME_SECTION(defaultTimer, "CJStreamMergerBase::buffCompare");
        return icmp->docompare(pending[mergeheap[a]], pending[mergeheap[b]]);
    }

    bool promote(unsigned p)
    {
        activeInputs--;
        if(activeInputs == p)
            return false;
        mergeheap[p] = mergeheap[activeInputs];
        return true;
    }

    inline bool siftDown(unsigned p)
    {
        //MTIME_SECTION(defaultTimer, "CJStreamMergerBase::siftDown");
        // assuming that all descendants of p form a heap, sift p down to its correct position, and so include it in the heap
        bool nochange = true;
        while(1)
        {
            unsigned c = p*2 + 1;
            if(c >= activeInputs) 
                return nochange;
            if(c+1 < activeInputs)
            {
                int childcmp = buffCompare(c+1, c);
                if((childcmp < 0) || ((childcmp == 0) && (mergeheap[c+1] < mergeheap[c])))
                    ++c;
            }
            int cmp = buffCompare(c, p);
            if((cmp > 0) || ((cmp == 0) && (mergeheap[c] > mergeheap[p])))
                return nochange;
            nochange = false;
            unsigned r = mergeheap[c];
            mergeheap[c] = mergeheap[p];
            mergeheap[p] = r;
            p = c;
        }
    }

    void siftDownDedupTop()
    {
        //MTIME_SECTION(defaultTimer, "CJStreamMergerBase::siftDownDedupTop");
        // same as siftDown(0), except that it also ensures that the top of the heap is not equal to either of its children
        if(activeInputs < 2)
            return;
        unsigned c = 1;
        int childcmp = 1;
        if(activeInputs >= 3)
        {
            childcmp = buffCompare(2, 1);
            if(childcmp < 0)
                c = 2;
        }
        int cmp = buffCompare(c, 0);
        if(cmp > 0)
            return;
        // the following loop ensures the correct property holds on the smaller branch, and that childcmp==0 iff the top matches the other branch
        while(cmp <= 0)
        {
            if(cmp == 0)
            {
                if(mergeheap[c] < mergeheap[0])
                {
                    unsigned r = mergeheap[c];
                    mergeheap[c] = mergeheap[0];
                    mergeheap[0] = r;
                }
                if(!pullInput(mergeheap[c]))
                    if(!promote(c))
                        break;
                siftDown(c);
            }
            else
            {
                unsigned r = mergeheap[c];
                mergeheap[c] = mergeheap[0];
                mergeheap[0] = r;
                if(siftDown(c))
                    break;
            }
            cmp = buffCompare(c, 0);
        }
        // the following loop ensures the uniqueness property holds on the other branch too
        c = 3-c;
        if(activeInputs <= c)
            return;
        while(childcmp == 0)
        {
            if(mergeheap[c] < mergeheap[0])
            {
                unsigned r = mergeheap[c];
                mergeheap[c] = mergeheap[0];
                mergeheap[0] = r;
            }
            if(!pullInput(mergeheap[c]))
                if(!promote(c))
                    break;
            siftDown(c);
            childcmp = buffCompare(c, 0);
        }
    }


    void init()
    {
        if (activeInputs>0) {
            // setup heap property
            if (activeInputs >= 2)
                for(unsigned p = (activeInputs-2)/2; p > 0; --p)
                    siftDown(p);
            if (partdedup)
                siftDownDedupTop();
            else
                siftDown(0);
        }
        recno = 0;
    }

    inline count_t num() const { return recno; }

    
    inline bool _next()
    {
        if (!activeInputs)
            return false;
        if (recno) {
            if(!pullInput(mergeheap[0]))
                if(!promote(0))
                    return false;
            // we have changed the element at the top of the heap, so need to sift it down to maintain the heap property
            if(partdedup)
                siftDownDedupTop();
            else
                siftDown(0);
        }
        recno++;
        return true;
    }
    inline bool eof() const { return activeInputs==0; }


    bool pullInput(unsigned i)
    {
        if (pending[i]) {
            assertex(partdedup);
            provider.releaseRow(pending[i]);
        }
        pending[i] = provider.nextRow(i);
        if (pending[i])
            return true;
        provider.stop(i);
#ifdef _DEBUG
        assertex(!stopped[i]);
        stopped[i] = true;
#endif
        return false;
    }

public:
    CRowStreamMerger(IRowProvider &_provider,unsigned numstreams, const ICompare *_icmp,bool _partdedup=false)
        : provider(_provider)
    {
        partdedup = _partdedup;
        icmp = _icmp;
        recsize = NULL;
        activeInputs = 0;
#ifdef _DEBUG
        stopped = (bool *)ma.allocate(numstreams*sizeof(bool));
        memset(stopped,0,numstreams*sizeof(bool));
#endif
        unsigned i;
        recsize = NULL;
        if (numstreams) {
            byte *buf = (byte *)workingbuf.allocate(numstreams*(sizeof(void *)+sizeof(unsigned)));
            pending = (const void **)buf;
            mergeheap = (unsigned *)(pending+numstreams);
            for (i=0;i<numstreams;i++) {
                pending[i] = NULL;
                if (pullInput(i)) 
                    mergeheap[activeInputs++] = i;
            }
        }
        else {
            pending = NULL;
            mergeheap = NULL;
        }
        init();
    }
    void stop()
    {
        while (activeInputs) {
            activeInputs--;
            if (pending[mergeheap[activeInputs]]) {
                provider.releaseRow(pending[mergeheap[activeInputs]]);
#ifdef _DEBUG
                assertex(!stopped[mergeheap[activeInputs]]);
                stopped[mergeheap[activeInputs]] = true;
#endif
                provider.stop(mergeheap[activeInputs]);
            }
        }
        pending = NULL;
        mergeheap = NULL;
        workingbuf.clear();
    }

    ~CRowStreamMerger()
    {
        stop();
    }

    inline const void * next()
    {
        if (!_next())
            return NULL;
        unsigned strm = mergeheap[0];
        const void *row = pending[strm];
        pending[strm] = NULL;
        return row;
    }

};


class CMergeRowStreams : public CInterface, implements IRowStream
{
protected:
    CRowStreamMerger *merger;
    bool eos;

    class cProvider: public CInterface, implements IRowProvider
    {
        IArrayOf<IRowStream> ostreams;
        IRowStream **streams;
        Linked<IRowLinkCounter> linkcounter;
        const void *nextRow(unsigned idx)
        {
            return streams[idx]->nextRow();
        };
        void linkRow(const void *row)
        {
            linkcounter->linkRow(row);
        }
        void releaseRow(const void *row)
        {
            linkcounter->releaseRow(row);
        }
        void stop(unsigned idx)
        {
            streams[idx]->stop();
        }
    public:
        IMPLEMENT_IINTERFACE;
        cProvider(IRowStream **_streams, unsigned numstreams, IRowLinkCounter *_linkcounter)
            : linkcounter(_linkcounter)
        {
            ostreams.ensure(numstreams);
            unsigned n = 0;
            while (n<numstreams) 
                ostreams.append(*LINK(_streams[n++]));
            streams = ostreams.getArray();
        }
    } *streamprovider;

    
public:
    CMergeRowStreams(unsigned _numstreams,IRowStream **_instreams,ICompare *_icmp, bool partdedup, IRowLinkCounter *_linkcounter)
    {
        streamprovider = new cProvider(_instreams, _numstreams, _linkcounter);
        merger = new CRowStreamMerger(*streamprovider,_numstreams,_icmp,partdedup);
        eos = _numstreams==0;
        
    }

    CMergeRowStreams(unsigned _numstreams,IRowProvider &_provider,ICompare *_icmp, bool partdedup)
    {
      streamprovider = NULL;
        merger = new CRowStreamMerger(_provider,_numstreams,_icmp,partdedup);
        eos = _numstreams==0;
    }


    ~CMergeRowStreams()
    {
        delete merger;
        delete streamprovider;
    }


    IMPLEMENT_IINTERFACE;

    void stop()
    {
        if (!eos) {
            merger->stop();
            eos = true;
        }
    }

    const void *nextRow() 
    {
        if (eos)
            return NULL;
        const void *r = merger->next();
        if (!r) {
            stop(); // think ok to stop early
            return NULL;
        }
        return r;
    }

};


IRowStream *createRowStreamMerger(unsigned numstreams,IRowStream **instreams,ICompare *icmp,bool partdedup,IRowLinkCounter *linkcounter)
{
    return new CMergeRowStreams(numstreams,instreams,icmp,partdedup,linkcounter);
}


IRowStream *createRowStreamMerger(unsigned numstreams,IRowProvider &provider,ICompare *icmp,bool partdedup)
{
    return new CMergeRowStreams(numstreams,provider,icmp,partdedup);
}
