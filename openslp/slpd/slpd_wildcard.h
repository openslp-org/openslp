

#if(!defined SLPD_WILDCARD_H_INCLUDED)
#define SLPD_WILDCARD_H_INCLUDED

/* Returns
 * 0 success
 * 1 failure
 */
int wildcard(const char *pattern, const char *string);

#endif /* SLPD_WILDCARD_H_INCLUDED */
