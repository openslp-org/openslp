#include "slp_attr.h"
#include "slp_filter.h"

#ifdef DEBUG
void dumpAttrList(int level, const SLPAttrList *attrs);
void dumpFilterTree(const SLPLDAPFilter *filter);
#endif

int SLP_predicate_match(const SLPAttrList *a, const char *b);

