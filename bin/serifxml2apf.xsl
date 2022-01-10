<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="2.0"
		xmlns:str="http://exslt.org/strings"
		xmlns:xalan="http://xml.apache.org/xalan">
   <xsl:output method="xml" indent="yes"/>

   <xsl:template match="/">

     <xsl:variable name="doc_id">
       <xsl:value-of select="SerifXML/Document/@docid"/>
     </xsl:variable>

     <!--<source_file SOURCE="newswire" TYPE="text" VERSION="2.0" URI="$doc_id">-->
     <xsl:element name="source_file">    
       <xsl:attribute name="SOURCE">newswire</xsl:attribute>
       <xsl:attribute name="TYPE">text</xsl:attribute>
       <xsl:attribute name="VERSION">2.0</xsl:attribute>
       <xsl:attribute name="URI"><xsl:copy-of select="$doc_id"/></xsl:attribute>

       <!--<document DOCID="$doc_id">-->
       <xsl:element name="document">
	 <xsl:attribute name="DOCID"><xsl:copy-of select="$doc_id"/></xsl:attribute>

	 <!-- ENTITIES -->
	 <xsl:for-each select="//Entity">
	   <xsl:element name="entity">
	     <xsl:attribute name="ID"><xsl:value-of select="$doc_id"/>-<xsl:value-of select="@id"/></xsl:attribute>
	     <xsl:attribute name="TYPE"><xsl:value-of select="@entity_type"/></xsl:attribute>
	     <xsl:attribute name="SUBTYPE"><xsl:value-of select="@entity_subtype"/></xsl:attribute>
	     <xsl:attribute name="CLASS">
	       <xsl:choose>
		 <xsl:when test="@is_generic = TRUE">GEN</xsl:when>
		 <xsl:otherwise>SPC</xsl:otherwise>
	       </xsl:choose>
	     </xsl:attribute>

	     <xsl:variable name="mention_ids" select="str:split(@mention_ids)"/>
	     <xsl:for-each select="//Mention[@id=$mention_ids]">

	       <xsl:if test="@mention_type='name' or @mention_type='desc' or @mention_type='pron' or @mention_type='part'">
		 <xsl:element name="entity_mention">
		   <xsl:variable name="syn_node_id" select="@syn_node_id"/>
		   <xsl:attribute name="TYPE">
		     <xsl:choose>
		       <xsl:when test="@mention_type='name'">NAM</xsl:when>
		       <xsl:when test="@mention_type='desc'">NOM</xsl:when>
		       <xsl:when test="@mention_type='pron' or @mention_type='part'">PRO</xsl:when>
		       <xsl:otherwise>BAD_VALUE</xsl:otherwise>  
		     </xsl:choose>
		   </xsl:attribute>
		   <xsl:attribute name="ID"><xsl:value-of select="$doc_id"/>-<xsl:value-of select="@id"/></xsl:attribute>
		   <xsl:if test="@role_type!='UNDET'">
		     <xsl:attribute name="ROLE"><xsl:value-of select="@role_type"/></xsl:attribute>
		   </xsl:if>
		 
		   <xsl:element name="extent">
		     <xsl:call-template name="print_charseq">
		       <xsl:with-param name="elem" select="//SynNode[@id=$syn_node_id]"/>
		     </xsl:call-template>
		   </xsl:element>
		
		   <xsl:element name="head">
		     <xsl:call-template name="print_head_charseq_for_mention">
		       <xsl:with-param name="mention" select="."/>
		     </xsl:call-template>
		   </xsl:element>

		 </xsl:element>
	       </xsl:if>
	     </xsl:for-each>

	     <xsl:variable name="named_mentions" select="//Mention[@id=$mention_ids and @mention_type='name']"/>
	     <xsl:if test="count($named_mentions) &gt; 0">
	       <xsl:element name="entity_attributes">
		 <xsl:for-each select="$named_mentions">
		   <xsl:element name="name">
		     <xsl:attribute name="NAME">
		       <xsl:call-template name="print_name_text">
			 <xsl:with-param name="mention" select="."/>
		       </xsl:call-template>
		     </xsl:attribute>
		     <xsl:call-template name="print_head_charseq_for_mention">
		       <xsl:with-param name="mention" select="."/>
		     </xsl:call-template>
		   </xsl:element>
		 </xsl:for-each>
	       </xsl:element>
	     </xsl:if>
		 
	   </xsl:element>
	 </xsl:for-each>

	 
	 <!-- TIMEX2 -->
	 <xsl:for-each select="//Value[@type='TIMEX2.TIME']">
	   <xsl:call-template name="print_timex2">
	     <xsl:with-param name="value" select="."/>
	     <xsl:with-param name="doc_id" select="$doc_id"/>
	   </xsl:call-template>
	 </xsl:for-each>

	 <!-- still need to handle datetime -->

	 <!-- RELATIONS -->
	 <xsl:for-each select="//Relation">
	   <xsl:variable name="type_string" select="str:split(@type, '.')[1]"/>
	   <xsl:variable name="subtype_string" select="str:split(@type, '.')[2]"/>
	   <xsl:element name="relation">
	     <xsl:attribute name="ID"><xsl:value-of select="$doc_id"/>-<xsl:value-of select="@id"/></xsl:attribute>
	     <xsl:attribute name="TYPE"><xsl:value-of select="$type_string"/></xsl:attribute>
	     <xsl:attribute name="SUBTYPE"><xsl:value-of select="$subtype_string"/></xsl:attribute>
	     <xsl:attribute name="MODALITY"><xsl:value-of select="@modality"/></xsl:attribute>
	     <xsl:attribute name="TENSE"><xsl:value-of select="@tense"/></xsl:attribute>
	     
	     <xsl:element name="relation_argument">
	       <xsl:attribute name="REFID"><xsl:value-of select="$doc_id"/>-<xsl:value-of select="@left_entity_id"/></xsl:attribute>
	       <xsl:attribute name="ROLE">Arg-1</xsl:attribute>
	     </xsl:element>

	     <xsl:element name="relation_argument">
	       <xsl:attribute name="REFID"><xsl:value-of select="$doc_id"/>-<xsl:value-of select="@right_entity_id"/></xsl:attribute>
	       <xsl:attribute name="ROLE">Arg-2</xsl:attribute>
	     </xsl:element>

	     <xsl:variable name="rel_mention_ids" select="str:split(@rel_mention_ids)"/>
	     <xsl:for-each select="//RelMention[@id=$rel_mention_ids]">
	       <xsl:element name="relation_mention">
		 <xsl:attribute name="ID"><xsl:value-of select="$doc_id"/>-<xsl:value-of select="@id"/></xsl:attribute>
		 <xsl:element name="extent"> <!-- we're just using the left mention as the extent -->
		   <xsl:variable name="left_mention_id" select="@left_mention_id"/>
		   <xsl:variable name="syn_node_id" select="//Mention[@id=$left_mention_id]/@syn_node_id"/>
		   <xsl:call-template name="print_charseq">
		     <xsl:with-param name="elem" select="//SynNode[@id=$syn_node_id]"/>
		   </xsl:call-template>
		 </xsl:element>
		 <xsl:call-template name="print_rel_mention_arg">
		   <xsl:with-param name="mention_id" select="@left_mention_id"/>
		   <xsl:with-param name="role" select="'Arg-1'"/>
		   <xsl:with-param name="doc_id" select="$doc_id"/>
		 </xsl:call-template>
		 <xsl:call-template name="print_rel_mention_arg">
		   <xsl:with-param name="mention_id" select="@right_mention_id"/>
		   <xsl:with-param name="role" select="'Arg-2'"/>
		   <xsl:with-param name="doc_id" select="$doc_id"/>
		 </xsl:call-template>
	       </xsl:element>
	     </xsl:for-each>
	   </xsl:element>
	 </xsl:for-each>

	 <!-- EVENTS -->
	 <xsl:for-each select="//Event">
	   <xsl:variable name="type_string" select="str:split(@event_type, '.')[1]"/>
	   <xsl:variable name="subtype_string" select="str:split(@event_type, '.')[2]"/>
	   <xsl:element name="event">
	     <xsl:attribute name="ID"><xsl:value-of select="$doc_id"/>-<xsl:value-of select="@id"/></xsl:attribute>
	     <xsl:attribute name="TYPE"><xsl:value-of select="$type_string"/></xsl:attribute>
	     <xsl:attribute name="SUBTYPE"><xsl:value-of select="$subtype_string"/></xsl:attribute>
	     <xsl:attribute name="MODALITY"><xsl:value-of select="@modality"/></xsl:attribute>
	     <xsl:attribute name="POLARITY"><xsl:value-of select="@polarity"/></xsl:attribute>
	     <xsl:attribute name="GENERICITY"><xsl:value-of select="@genericity"/></xsl:attribute>
	     <xsl:attribute name="TENSE"><xsl:value-of select="@tense"/></xsl:attribute>

	     <xsl:for-each select="./EventArg">
	       <xsl:element name="event_argument">
		 <xsl:attribute name="REFID"><xsl:value-of select="$doc_id"/>-<xsl:value-of select="@entity_guid|@value_id"/></xsl:attribute>
		 <xsl:attribute name="ROLE"><xsl:value-of select="@role"/></xsl:attribute>
	       </xsl:element>
	     </xsl:for-each>

	     <xsl:variable name="event_mention_ids" select="str:split(@event_mention_ids)"/>
	     <xsl:for-each select="//EventMention[@id=$event_mention_ids]">
	       <xsl:variable name="event_mention_id" select="@id"/>
	       <xsl:element name="event_mention">
		 <xsl:attribute name="ID"><xsl:value-of select="$doc_id"/>-<xsl:value-of select="$event_mention_id"/></xsl:attribute>
		 <xsl:variable name="event_mention" select="."/>
		 <xsl:variable name="anchor_node" select="@anchor_node_id"/>
		 <xsl:element name="extent">
		   <xsl:call-template name="print_charseq">
		     <xsl:with-param name="elem" select="//SynNode[@id=$anchor_node]"/>
		   </xsl:call-template>
		 </xsl:element>
		 <xsl:element name="anchor">
		   <xsl:call-template name="print_charseq">
		     <xsl:with-param name="elem" select="//SynNode[@id=$anchor_node]"/>
		   </xsl:call-template>
		 </xsl:element>
		 <xsl:for-each select="./EventMentionArg">
		   <xsl:element name="event_mention_argument">
		     <xsl:attribute name="REFID"><xsl:value-of select="$doc_id"/>-<xsl:value-of select="@mention_id|@value_mention_id"/></xsl:attribute>
		     <xsl:attribute name="ROLE"><xsl:value-of select="@role"/></xsl:attribute>
		     <xsl:element name="extent">
		       <xsl:variable name="mention_id" select="@mention_id|@value_mention_id"/>
		       <xsl:variable name="syn_node_id" select="//Mention[@id=$mention_id]/@syn_node_id"/>
		       <xsl:call-template name="print_charseq">
			 <xsl:with-param name="elem" select="//SynNode[@id=$syn_node_id]|//ValueMention[@id=$mention_id]"/>
		       </xsl:call-template>
		     </xsl:element>
		   </xsl:element>
		   </xsl:for-each>
	       </xsl:element>
	     </xsl:for-each>

	   </xsl:element>
	 </xsl:for-each>

       </xsl:element>
     </xsl:element>
   </xsl:template>

   <xsl:template name="print_rel_mention_arg">
     <xsl:param name="mention_id"/>
     <xsl:param name="role"/>
     <xsl:param name="doc_id"/>
     <xsl:element name="relation_mention_argument">
       <xsl:attribute name="REFID"><xsl:value-of select="$doc_id"/>-<xsl:value-of select="$mention_id"/></xsl:attribute>
       <xsl:attribute name="ROLE"><xsl:value-of select="$role"/></xsl:attribute>
       <xsl:element name="extent">
	 <xsl:variable name="syn_node_id" select="//Mention[@id=$mention_id]/@syn_node_id"/>
	 <xsl:call-template name="print_charseq">
	   <xsl:with-param name="elem" select="//SynNode[@id=$syn_node_id]"/>
	 </xsl:call-template>
       </xsl:element>
     </xsl:element>
   </xsl:template>
   
   <xsl:template name="print_timex2">
     <xsl:param name="value"/>
     <xsl:param name="doc_id"/>
     <xsl:element name="timex2">
       <xsl:attribute name="ID"><xsl:value-of select="$doc_id"/>-<xsl:value-of select="$value/@id"/></xsl:attribute>
       <xsl:attribute name="VAL"><xsl:value-of select="$value/@timex_val"/></xsl:attribute>
       <xsl:if test="count($value/@timex_anchor_val) &gt; 0">
	 <xsl:attribute name="ANCHOR_VAL"><xsl:value-of select="$value/@timex_anchor_val"/></xsl:attribute>
       </xsl:if>
       <xsl:if test="count($value/@timex_anchor_dir) &gt; 0">
	 <xsl:attribute name="ANCHOR_DIR"><xsl:value-of select="$value/@timex_anchor_dir"/></xsl:attribute>
       </xsl:if>
       <xsl:variable name="value_mention_ref" select="$value/@value_mention_ref"/>
       <xsl:for-each select="//ValueMention[@id=$value_mention_ref]">
	 <xsl:element name="timex2_mention">
	   <xsl:attribute name="ID"><xsl:value-of select="$doc_id"/>-<xsl:value-of select="@id"/></xsl:attribute>
	   <xsl:element name="extent">
	     <xsl:call-template name="print_charseq">
	       <xsl:with-param name="elem" select="."/>
	     </xsl:call-template>
	   </xsl:element>
	 </xsl:element>
       </xsl:for-each>
     </xsl:element>
   </xsl:template>


   <xsl:template name="print_head_charseq_for_mention">
     <xsl:param name="mention"/>
     <xsl:variable name="syn_node_id" select="$mention/@syn_node_id"/>
     <xsl:variable name="synnode" select="//SynNode[@id=$syn_node_id]"/>
     
     <xsl:choose>

       <!-- NAME or NONE (child of a NAME) -->
       <xsl:when test="$mention/@mention_type='name' or $mention/@mention_type='none'">
	 <xsl:choose>
	   <xsl:when test="count($mention/@child) &gt; 0">
	     <xsl:variable name="child_id" select="$mention/@child"/>
	     <xsl:call-template name="print_head_charseq_for_mention">
	       <xsl:with-param name="mention" select="//Mention[@id=$child_id]"/>
	     </xsl:call-template>
	   </xsl:when>
	   <xsl:otherwise>
	     <xsl:call-template name="print_charseq">
	       <xsl:with-param name="elem" select="$synnode"/>
	     </xsl:call-template>
	   </xsl:otherwise>
	 </xsl:choose>
       </xsl:when>

       <!-- ALL OTHERS -->
       <!-- If synnode is a preterminal, it is its own head. Otherwise, find head. --> 
       <xsl:otherwise>
	 <xsl:call-template name="print_head_charseq_for_node">
	   <xsl:with-param name="node" select="$synnode"/>
	 </xsl:call-template>
       </xsl:otherwise>

     </xsl:choose>
   </xsl:template>

   <xsl:template name="print_head_charseq_for_node">
     <xsl:param name="node"/>
     <xsl:variable name="id" select="$node/@id"/>
     <xsl:variable name="children" select="$node/SynNode"/>
     <xsl:variable name="grandchildren" select="$children/SynNode"/>	
     <xsl:variable name="mention" select="//Mention[@syn_node_id=$id and @mention_type='name']"/>
     
     <xsl:choose>
       <xsl:when test="count($children)=1 and count($grandchildren)=0">  <!-- node is a preterminal -->	
	 <xsl:call-template name="print_charseq">
	   <xsl:with-param name="elem" select="$node"/>
	 </xsl:call-template>
       </xsl:when>
       <xsl:when test="count($mention)=1">                               <!-- node has a mention -->
	 <xsl:call-template name="print_head_charseq_for_mention">
	   <xsl:with-param name="mention" select="$mention"/>
	 </xsl:call-template>
       </xsl:when>
       <xsl:otherwise>
	 <xsl:variable name="head" select="$children[@is_head='TRUE']"/>
	 <xsl:call-template name="print_head_charseq_for_node">
	   <xsl:with-param name="node" select="$head"/>
	 </xsl:call-template>
       </xsl:otherwise>
     </xsl:choose>
   </xsl:template>


   <xsl:template name="print_charseq">
     <xsl:param name="elem"/>

     <xsl:variable name="start_tok_id" select="$elem/@start_token"/>
     <xsl:variable name="end_tok_id" select="$elem/@end_token"/>
     <xsl:variable name="start_tok" select="//Token[@id=$start_tok_id]"/>
     <xsl:variable name="end_tok" select="//Token[@id=$end_tok_id]"/>
     <xsl:variable name="edt_start_offset_pair" select="$start_tok/@edt_offsets"/>
     <xsl:variable name="edt_start_offset" select="str:split($edt_start_offset_pair, ':')[1]"/>
     <xsl:variable name="edt_end_offset_pair" select="$end_tok/@edt_offsets"/>
     <xsl:variable name="edt_end_offset" select="str:split($edt_end_offset_pair, ':')[2]"/>
     <xsl:variable name="char_start_offset_pair" select="$start_tok/@char_offsets"/>
     <xsl:variable name="char_start_offset" select="str:split($char_start_offset_pair, ':')[1]"/>
     <xsl:variable name="char_end_offset_pair" select="$end_tok/@char_offsets"/>
     <xsl:variable name="char_end_offset" select="str:split($char_end_offset_pair, ':')[2]"/>
     <xsl:variable name="document_text" select="/SerifXML/Document/OriginalText/Contents/text()"/>
     <xsl:variable name="document_string" select="string($document_text)"/>
     <xsl:variable name="substring_len" select="number($char_end_offset)-number($char_start_offset)+1"/>
     
     <xsl:element name="charseq">
       <xsl:attribute name="START"><xsl:copy-of select="$edt_start_offset"/></xsl:attribute>
       <xsl:attribute name="END"><xsl:copy-of select="$edt_end_offset"/></xsl:attribute>
       <xsl:call-template name="print_text">
	 <xsl:with-param name="start_tok_id" select="$start_tok_id"/>
	 <xsl:with-param name="end_tok_id" select="$end_tok_id"/>
       </xsl:call-template>
     </xsl:element>

   
   </xsl:template>
  
   <xsl:template name="print_name_text">
     <xsl:param name="mention"/>     
     <xsl:choose>
       <xsl:when test="count($mention/@child) &gt; 0">
	 <xsl:variable name="child_id" select="$mention/@child"/>
	 <xsl:call-template name="print_name_text">
	   <xsl:with-param name="mention" select="//Mention[@id=$child_id]"/>
	 </xsl:call-template>
       </xsl:when>
       <xsl:otherwise>
	 <xsl:variable name="syn_node_id" select="$mention/@syn_node_id"/>
	 <xsl:variable name="synnode" select="//SynNode[@id=$syn_node_id]"/>   
	 <xsl:call-template name="print_text">  
	   <xsl:with-param name="start_tok_id" select="$synnode/@start_token"/>
	   <xsl:with-param name="end_tok_id" select="$synnode/@end_token"/>
	 </xsl:call-template>
       </xsl:otherwise>
     </xsl:choose>
   </xsl:template>

   <xsl:template name="print_text">
     <xsl:param name="start_tok_id"/>
     <xsl:param name="end_tok_id"/>

     <xsl:variable name="document_text" select="string(/SerifXML/Document/OriginalText/Contents/text())"/>
     <xsl:variable name="start_tok" select="//Token[@id=$start_tok_id]"/>
     <xsl:variable name="end_tok" select="//Token[@id=$end_tok_id]"/>
     <xsl:variable name="char_start_offset_pair" select="$start_tok/@char_offsets"/>
     <xsl:variable name="char_start_offset" select="str:split($char_start_offset_pair, ':')[1]"/>
     <xsl:variable name="char_end_offset_pair" select="$end_tok/@char_offsets"/>
     <xsl:variable name="char_end_offset" select="str:split($char_end_offset_pair, ':')[2]"/>
     <xsl:variable name="substring_len" select="number($char_end_offset)-number($char_start_offset)+1"/>

     <xsl:value-of select="substring($document_text,$char_start_offset+1,$substring_len)"/>

   </xsl:template>

</xsl:stylesheet>
