
#include <string.h>

#include "platform.h"

#if CFG_HAVE_STRUPR==0
void strupr(char *pszString)
{
	if( pszString!=NULL )
	{
		while( *pszString!=0 )
		{
			*pszString = toupper(*pszString);
			++pszString;
		}
	}
}
#endif

