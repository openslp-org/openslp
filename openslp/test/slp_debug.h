/****************************************************************************/
/* slp_debug                                                                */
/* Creation Date: Wed May 24 14:26:50 EDT 2000                              */
/****************************************************************************/
#include <stdio.h>
#define MAX_STRING_LENGTH		4096

typedef struct {
	SLPError	error_number;
	char		*label;
	char		*description;
} SLPErrorEntry;

SLPErrorEntry error_entries[] = {
	SLP_LAST_CALL,
		 (char *) (const) "SLP_LAST_CALL",
		 (char *) (const) "Passed to callback functions when the API library has no more data for them and therefore no further calls will be made to the callback on the currently outstanding operation. The callback can use this to signal the main body of the client code that no more data will be forthcoming on the operation, so that the main body of the client code can break out of data collection loops. On the last call of a callback during both a synchronous and synchronous call, the error code parameter has value SLP_LAST_CALL, and the other parameters are all NULL. If no results are returned by an API operation, then only one call is made , with the error parameter set to SLP_LAST_CALL.",
	SLP_OK,
		 (char *) (const) "SLP_OK",
		 (char *) (const) "No DA or SA has service advertisement or attribute information in the language requested, but at least one DA or SA indicated, via the LANGUAGE_NOT_SUPPORTED error code, that it might have information for that service in another language",
	//SLP_LANGUAGE_NOT_SUPPORTED,
	-1,
		 (char *) (const) "SLP_LANGUAGE_NOT_SUPPORTED",
		 (char *) (const) "The SLP message was rejected by a remote SLP agent. The API returns this error only when no information was retrieved, and at least one SA or DA indicated a protocol error. The data supplied through the API may be malformed or a may have been damaged in transit.",
	SLP_INVALID_REGISTRATION,
		 (char *) (const) "SLP_INVALID_REGISTRATION",
		 (char *) (const) "The API may return this error if an attempt to register a service was rejected by all DAs because of a malformed URL or attributes. SLP does not return the error if at least one DA accepted the registration.", 
	SLP_AUTHENTICATION_ABSENT,
		 (char *) (const) "SLP_AUTHENTICATION_ABSENT",
		 (char *) (const) "The API returns this error if the SA has been configured with net.slp.useScopes value-list of scopes and the SA request did not specify one or more of these allowable scopes, and no others. It may be returned by a DA or SA if the scope included in a request is not supported by the DA or SA.", 
	SLP_INVALID_UPDATE, 
		 (char *) (const) "SLP_INVALID_UPDATE", 
		 (char *) (const) "if the SLP framework supports authentication, this error arises when the UA or SA failed to send an authenticator for requests or registrations in a protected scope.",
	SLP_AUTHENTICATION_FAILED, 
		 (char *) (const) "SLP_AUTHENTICATION_FAILED", 
		 (char *) (const) "If the SLP framework supports authentication, this error arises when a authentication on an SLP message failed",
	SLP_INVALID_UPDATE, 
		 (char *) (const) "SLP_INVALID_UPDATE", 
		 (char *) (const) "An update for a non-existing registration was issued, or the update includes a service type or scope different than that in the initial registration, etc.", 
	SLP_REFRESH_REJECTED, 
		 (char *) (const) "SLP_REFRESH_REJECTED", 
		 (char *) (const) "The SA attempted to refresh a registration more frequently than the minimum refresh interval. The SA should call the appropriate API function to obtain the minimum refresh interval to use.",
	SLP_NOT_IMPLEMENTED, 
		 (char *) (const) "SLP_NOT_IMPLEMENTED", 
		 (char *) (const) "If an unimplemented feature is used, this error is returned.",
	SLP_BUFFER_OVERFLOW,
		 (char *) (const) "SLP_BUFFER_OVERFLOW", 
		 (char *) (const) "An outgoing request overflowed the maximum network MTU size. The request should be reduced in size or broken into pieces and tried again.",
	SLP_NETWORK_TIMED_OUT,
		 (char *) (const) "SLP_NETWORK_TIMED_OUT", 
		 (char *) (const) "When no reply can be obtained in the time specified by the configured timeout interval for a unicast request, this error is returned.",
	SLP_NETWORK_INIT_FAILED, 
		 (char *) (const) "SLP_NETWORK_INIT_FAILED", 
		 (char *) (const) "If the network cannot initialize properly, this error is returned. Will also be returned if an SA or DA agent (slpd) can not be contacted. See SLPReg() and SLPDeReg() for more information.",
	SLP_MEMORY_ALLOC_FAILED, 
		 (char *) (const) "SLP_MEMORY_ALLOC_FAILED", 
		 (char *) (const) "Out of memory error",
	SLP_PARAMETER_BAD, 
		 (char *) (const) "SLP_PARAMETER_BAD", 
		 (char *) (const) "If a parameter passed into a function is bad, this error is returned.",
	SLP_NETWORK_ERROR, 
		 (char *) (const) "SLP_NETWORK_ERROR ", 
		 (char *) (const) "The failure of networking during normal operations causes this error to be returned.",
	SLP_INTERNAL_SYSTEM_ERROR, 
		 (char *) (const) "SLP_INTERNAL_SYSTEM_ERROR", 
		 (char *) (const) "A basic failure of the API causes this error to be returned. This occurs when a system call or library fails. The operation could not recover.",
	SLP_HANDLE_IN_USE, 
		 (char *) (const) "SLP_HANDLE_IN_USE", 
		 (char *) (const) "In the C API, callback functions are not permitted to recursively call into the API on the same SLPHandle, either directly or indirectly. If an attempt is made to do so, this error is returned from the called API function.",
	SLP_TYPE_ERROR, 
		 (char *) (const) "SLP_TYPE_ERROR", 
		 (char *) (const) "If the API supports type checking of registrations against service type templates, this error can arise if the attributes in a registration do not match the service type template for the service.",
};

/* These strings are returned if the error code is not found. */
#define UNKNOWN_ERROR_LABEL			"Unknown"
#define UNKNOWN_ERROR_DESCRIPTION	"Undefined error code."

/*=========================================================================*/
void get_full_error_data(int error_number,
		char **error_name,
		char **error_description)
/* Returns data in the parameter variables about the error code            */
/*                                                                         */
/* errorNumber -	Error code received.                                   */
/*                                                                         */
/* errorName -		Name of the error code.                                */
/*                                                                         */
/* errorDescription -		A long winded description about the error.     */
/*                                                                         */
/* Returns -		Nothing.                                               */
/*                                                                         */
/* Comment -		This returns (char *) (const) pointers to the strings  */
/*					which means that deletion of the strings is un-neces-  */
/*					ary.                                                   */
/*=========================================================================*/
{
	int		i;
	int		num_entires;

	/* Determine the number of entries in the error code array. */
	num_entires = sizeof(error_entries) / sizeof(SLPErrorEntry);
	for (i = 0; i < num_entires; i++)
	{
		if (error_entries[i].error_number == error_number)
		{
			*error_name = (error_entries[i].label);
			*error_description = (error_entries[i].description);
			return;
		} /* End If. */
	} /* End For. */
	*error_name = UNKNOWN_ERROR_LABEL;
	*error_description = UNKNOWN_ERROR_DESCRIPTION;
} /* End getFullErrorData(int, char *, char *). */

void check_error_state(int err, char *location_text)
{
	char		*error_name;
	char		*error_description;

	if (err != SLP_OK)
    {
        get_full_error_data(err, &error_name, &error_description);
        printf ("%s\n%d: %s\n%s\n",
            location_text, err, error_name, error_description);
        exit(err);
    } /* End If. */
}

