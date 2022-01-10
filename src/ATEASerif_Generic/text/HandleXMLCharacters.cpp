// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "HandleXMLCharacters.h"

void contract (char doc[], int start, int offset);

/*This routine walks through a document looking for special 
XML characters, replacing them with "regular" characters so
Serif counts each as a single character. The characters are:
	&lt;	<
	&gt;	>
	&amp;	&
	&apos;	'
	&quot;	"
Because Serif completely ignores tags, "&lt;" and "&gt;" are 
replaced with "#"
THIS ROUTINE ASSUMES A PROPERLY NULL (\0) TERMINATED STRING*/

void handleXMLcharacters (char doc[])
{
	int index;

	index = 0;

	while (doc[index] != '\0')
	{
		if (doc[index] == '/' && (index == 0 || doc[index - 1] == '\n')) 
			doc[index] = ' ';

		if (doc[index] == '&') //we may have an XML special character
		{
			if (((doc[index + 1] == 'l') ||
				 (doc[index + 1] == 'g')) &&
				(doc[index + 2] == 't') &&
				(doc[index + 3] == ';'))
			{
				doc[index] = ' ';
				contract (doc, index + 1, 3);
			}
			else if ((doc[index + 1] == 'a') &&
				     (doc[index + 2] == 'm') &&
					 (doc[index + 3] == 'p') &&
					 (doc[index + 4] == ';'))
			{
				doc[index] = '&';
				contract (doc, index + 1, 4);
			}
			else if ((doc[index + 1] == 'a') &&
				     (doc[index + 2] == 'p') &&
					 (doc[index + 3] == 'o') &&
					 (doc[index + 4] == 's') &&
					 (doc[index + 5] == ';'))
			{
				doc[index] = '\'';
				contract (doc, index + 1, 5);
			}
			else if ((doc[index + 1] == 'q') &&
				     (doc[index + 2] == 'u') &&
					 (doc[index + 3] == 'o') &&
					 (doc[index + 4] == 't') &&
					 (doc[index + 5] == ';'))
			{
				doc[index] = '\"';
				contract (doc, index + 1, 5);
			}
		}
		index++;
	}
}

void contract (char doc[], int start, int offset)
{
	int index;

	index = start;

	doc[index] = doc[index + offset];

	while (doc[index] != '\0')
	{
		index ++;
		doc[index] = doc[index + offset];
	}
}

