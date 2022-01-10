/*

  binsearch.c - general binary search functions

*/

#include "Generic/common/leak_detection.h"
#include <stdio.h>
#include <string.h>

/* Binary search - looks for the key passed at the start of a line
   in the file associated with open file descriptor fp, and returns
   a buffer containing the line in the file. */

#define KEY_LEN		(1024)
#ifdef UNIX
#define LINE_LEN	(1024*25)
#endif
#ifdef MAC
#define LINE_LEN	(1024*8)
#endif
#ifdef PC
#define LINE_LEN	(1024*8)
#endif

static char line[LINE_LEN]; 

/* General purpose binary search function to search for key as first
   item on line in open file.  Item is delimited by space. */

#undef getc

char *bin_search_memory (char *searchkey, char *fileArray, int arrayLength){ 
    int top, mid, bot, diff;   // long top, mid, bot, diff;
    char *linep, key[KEY_LEN];
    int length;
	int charPosition=0;
	char currentChar;
	int i;

    diff=666;
    linep = line;
    line[0] = '\0';

    
    top = 0;
	// need to debug 
    bot = arrayLength-1;
    mid = (bot - top) / 2;
	//printf ("array length %d -- the last few characters %c \n", arrayLength, fileArray[bot-6]);
	
    do {
		charPosition = mid-1;
		currentChar = fileArray[charPosition];
		if(mid != 1){
			while(currentChar != '\n' && currentChar != EOF){
				charPosition++;
				currentChar=fileArray[charPosition]; 
			}
			if (currentChar == '\n'){
				charPosition++;	
				currentChar=fileArray[charPosition]; 
			}
		}
		if (currentChar != EOF){
			i=0;
			while (currentChar != '\n' && currentChar != EOF ){
				linep[i++]=fileArray[charPosition++];
				currentChar = fileArray[charPosition];
			}
			linep[i]='\0';	

			length = (int)(strchr(linep, ' ') - linep);
			strncpy(key, linep, length);
			key[length] = '\0';
			if(strcmp(key, searchkey) < 0) {
				top = mid;
				diff = (bot - top) / 2;
				mid = top + diff;
			}
			if(strcmp(key, searchkey) > 0) {
				bot = mid;
				diff = (bot - top) / 2;
				mid = top + diff;
			}
		}else{
			break;
		}
	}while((strcmp(key, searchkey)) && (diff != 0));
	//printf ("stop checking at line %s, with key %s \n", linep, key);
	//printf ("finish binary search !!\n");
	//fflush(stdout);

	
	if(!strcmp(key, searchkey))
		return(line);
	else
		return(NULL);
}

char *bin_search(char *searchkey, FILE *fp)
{
    int c;
    long top, mid, bot, diff;
    char *linep, key[KEY_LEN];
    int length;

    diff=666;
    linep = line;
    line[0] = '\0';

    fseek(fp, 0L, 2);
    top = 0;
    bot = ftell(fp);
    mid = (bot - top) / 2;

    do {
	fseek(fp, mid - 1, 0);
	if(mid != 1)
	    while((c = getc(fp)) != '\n' && c != EOF);
	fgets(linep, LINE_LEN, fp);
	length = (int)(strchr(linep, ' ') - linep);
	strncpy(key, linep, length);
	key[length] = '\0';

	

	if(strcmp(key, searchkey) < 0) {
	    top = mid;
	    diff = (bot - top) / 2;
	    mid = top + diff;
	}
	if(strcmp(key, searchkey) > 0) {
	    bot = mid;
	    diff = (bot - top) / 2;
	    mid = top + diff;
	}
    } while((strcmp(key, searchkey)) && (diff != 0));
    
    if(!strcmp(key, searchkey))
	return(line);
    else
	return(NULL);
}

static long offset;

static int bin_search_key(char *searchkey, FILE *fp)
{
    int c;
    long top, mid, bot, diff;
    char *linep, key[KEY_LEN];
    int length, offset1, offset2;

    /* do binary search to find correct place in file to insert line */

    diff=666;
    linep = line;
    line[0] = '\0';

    fseek(fp, 0L, 2);
    top = 0;
    bot = ftell(fp);
    if (bot == 0) {
	offset = 0;
	return(0);		/* empty file */
    }
    mid = (bot - top) / 2;

    /* If only one line in file, don't work through loop */

    length = 0;
    rewind(fp);
    while((c = getc(fp)) != '\n' && c != EOF)
	line[length++] = (char)c;
    if (getc(fp) == EOF) {	/* only 1 line in file */
	length = (int)(strchr(linep, ' ') - linep);
	strncpy(key, linep, length);
	key[length] = '\0';
	if(strcmp(key, searchkey) > 0) {
	    offset = 0;
	    return(0);		/* line with key is not found */
	} else if (strcmp(key, searchkey) < 0) {
	    offset = ftell(fp);
	    return(0);		/* line with key is not found */
	} else {
	    offset = 0;
	    return(1);		/* line with key is found */
	}
    }

    do {
	fseek(fp, mid - 1, 0);
	if(mid != 1)
	    while((c = getc(fp)) != '\n' && c != EOF);
	offset1 = ftell(fp);	/* offset at start of line */
	if (fgets(linep, LINE_LEN, fp) != NULL) {
  	    offset2 = ftell(fp); /* offset at start of next line */
	    length = (int)(strchr(linep, ' ') - linep);
	    strncpy(key, linep, length);
	    key[length] = '\0';
	    if(strcmp(key, searchkey) < 0) {	/* further in file */
		top = mid;
		diff = (bot - top) / 2;
		mid = top + diff;
		offset = offset2;
	    }
	    if(strcmp(key, searchkey) > 0) {	/* earlier in file */
		bot = mid;
		diff = (bot - top) / 2;
		mid = top + diff;
		offset = offset1;
	    }
	} else {
	    bot = mid;
	    diff = (bot - top) / 2;
	    mid = top + diff;
	}
    } while((strcmp(key, searchkey)) && (diff != 0));

    if(!strcmp(key, searchkey)) {
	offset = offset1;	/* get to start of current line */
	return(1);		/* line with key is found */
    } else
	return(0);		/* line with key is not found */
}

/* Copy contents from one file to another. */

void copyfile(FILE *fromfp, FILE *tofp)
{
    int c;

    while ((c = getc(fromfp)) != EOF)
	putc(c, tofp);
}

/* Function to replace a line in a file.  Returns the original line,
   or NULL in case of error. */

char *replace_line(char *new_line, char *searchkey, FILE *fp)
{
    FILE *tfp;			/* temporary file pointer */

    if (!bin_search_key(searchkey, fp))
	return(NULL);		/* line with key not found */

    if ((tfp = tmpfile()) == NULL)
	return(NULL);		/* could not create temp file */
    fseek(fp, offset, 0);
    fgets(line, LINE_LEN, fp);	/* read original */
    copyfile(fp, tfp);
    if (fseek(fp, offset, 0) == -1)
	return(NULL);		/* could not seek to offset */
    fprintf(fp, "%s", new_line);	/* write line */
    rewind(tfp);
    copyfile(tfp, fp);

    fclose(tfp);
    fflush(fp);

    return(line);
}

/* Find location to insert line at in file.  If line with this
   key is already in file, return NULL. */

char *insert_line(char *new_line, char *searchkey, FILE *fp)
{
    FILE *tfp;

    if (bin_search_key(searchkey, fp))
	return(NULL);
    
    if ((tfp = tmpfile()) == NULL)
	return(NULL);		/* could not create temp file */
    if (fseek(fp, offset, 0) == -1)
	return(NULL);		/* could not seek to offset */
    copyfile(fp, tfp);
    if (fseek(fp, offset, 0) == -1)
	return(NULL);		/* could not seek to offset */
    fprintf(fp, "%s", new_line);	/* write line */
    rewind(tfp);
    copyfile(tfp, fp);

    fclose(tfp);
    fflush(fp);

    return(new_line);
}
