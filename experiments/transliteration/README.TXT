The remove_dups.py script is used to remove duplicates from the name
lists that are used for transcription.  These name lists were
originally generated from the WKDB_1010 database (distill) using the
following SQL queries:

Arabic Interlinks:

  SELECT DISTINCT S.ar_page_title, E.page_title 
  FROM wiki_ar_interlinks S, wiki_en_pages E
  WHERE S.en_page_id = E.page_id ORDER BY 1

Chinese Interlinks:

  SELECT DISTINCT S.zh_page_title, E.page_title 
  FROM wiki_zh_interlinks S, wiki_en_pages E
  WHERE S.en_page_id = E.page_id ORDER BY 1

The remove_dups.py script was then used to resolve duplicates by
picking the target name that occurs most frequently in an English
corpus.
