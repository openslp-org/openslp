#if(!defined SLPD_PREDICATE_H_INCLUDED)
#define SLPD_PREDICATE_H_INCLUDED

#include <libslpattr.h>
#include <slp.h>

typedef void * SLPDPredicate ; 

SLPError SLPDPredicateAlloc(const char *predicate, size_t len, SLPDPredicate *pred);
void SLPDPredicateFree(SLPDPredicate *victim);

/*=========================================================================*/
int SLPTestPredicate(SLPDPredicate predicate, SLPAttributes attr);
/* Determine whether the specified attribute list satisfies                */
/* the specified predicate                                                 */
/*                                                                         */
/* predicatelen (IN) the length of the predicate string                    */
/*                                                                         */
/* predicate    (IN) the predicate string                                  */
/*                                                                         */
/* attr         (IN) attribute list to test                                */
/*                                                                         */
/* Returns: Zero if there is a match, a positive value if there was not a  */
/*          match, and a negative value if there was a parse error in the  */
/*          predicate string.                                              */
/*=========================================================================*/

#endif /* SLPD_PREDICATE_H_INCLUDED */
