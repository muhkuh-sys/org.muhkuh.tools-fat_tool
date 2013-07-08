
#include <string.h>

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