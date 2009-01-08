#include "utils.h"





char *replace_tokens(const char *src, const char** from, char** to, int n)
{

  int j;
  char *p1, *p2;

  p1 = g_strdup(src);

  for (j=0;j<n;j++)
    {
      p2 = replace( p1, from[j], to[j] );
      g_free(p1);
      p1 = p2;
    }

  return p1;

}


char *replace(const char *src, const char *from, const char *to)
{
  
  size_t size = strlen(src) + 1;
  size_t fromlen = strlen(from);
  size_t tolen = strlen(to);

  char *value = g_malloc(size);

  char *dst = value;

  if ( value != NULL )

    {

      for ( ;; )

	{

	  const char *match = strstr(src, from);

	  if ( match != NULL )
	    {
	      size_t count = match - src;
	      char *temp;
	      
	      size += tolen - fromlen;
	      temp = g_realloc(value, size);
	      
	      if ( temp == NULL )
		{
		  g_free(value);
		  return NULL;
		}

	      dst = temp + (dst - value);
	      value = temp;
	      memmove(dst, src, count);
	      src += count;
	      dst += count;

	      memmove(dst, to, tolen);
	      src += fromlen;
	      dst += tolen;

	    } else /* No match found. */
	    {
	    
	      strcpy(dst, src);
	      break;

	    }

	}
      
    }

  return value;

}
