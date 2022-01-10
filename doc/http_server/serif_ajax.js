
var result_counter = 0;

function getSelectValue(id) {
    return document.getElementById(id).options[document.getElementById(id).selectedIndex].value;
}

function sendDoc() {  
    // Construct the request.
    var end_stage = getSelectValue("end_stage");
    var offset_type = getSelectValue("offset_type"); 
    var input_type = getSelectValue("input_type"); 
    var xml_ids = getSelectValue("xml_ids");
    var parse_encoding = getSelectValue("parse_encoding");
    var result_format = getSelectValue("result_format");
    var output_format = getSelectValue("output_format");
    var doc_text = escapetext(document.getElementById("input-box").value);

    var verbose_ids = "false";
    var hierarchical_ids = "false"
    if (xml_ids == "verbose") { verbose_ids = "true"; }
    if (xml_ids == "hierarchical") { 
	hierarchical_ids = "true"; verbose_ids = "true"; }

    var include_spans_as = getSelectValue("include_spans_as");
    var include_spans_as_comments = (include_spans_as=="comments");
    var include_spans_as_elements = (include_spans_as=="elements");
    var include_mentions_as = getSelectValue("include_mentions_as");
    var include_mentions_as_comments = (include_mentions_as=="comments");
    var include_mentions_as_elements = (include_mentions_as=="elements");
    
    var request = 
        "<SerifXMLRequest>\n"+
        "  <ProcessDocument end_stage=\"" + end_stage + 
	"\" output_format=\"" + output_format +
	"\" verbose_ids=\"" + verbose_ids + 
	"\" hierarchical_ids=\"" + hierarchical_ids + 
	"\" input_type=\"" + input_type + 
	"\" parse_encoding=\"" + parse_encoding + 
	"\" include_spans_as_comments=\"" + include_spans_as_comments +
	"\" include_spans_as_elements=\"" + include_spans_as_elements +
	"\" include_mentions_as_comments=\"" + include_mentions_as_comments +
	"\" include_mentions_as_elements=\"" + include_mentions_as_elements +
	"\" offset_type=\"" + offset_type + "\">\n" + 
        "    <Document docid=\"testdoc.txt\" language=\"English\">\n" +
	"      <OriginalText>\n" +
	"        <Contents>" + doc_text + "</Contents>\n" +
        "      </OriginalText>\n" +
	"    </Document>\n" +
        "  </ProcessDocument>\n" + 
        "</SerifXMLRequest>\n";
    //alert(request);
    
    var results_div = document.getElementById("results-div");

    // Clear out any previous results.  (Alternatively, we could
    // keep old results.)
    results_div.innerHTML = "";

    if (result_format == "xml") {
	// Add a <pre> containing the request.
	var request_box = document.createElement("pre");
	request_box.className = "example-request";
	request_box.innerHTML = 
	    "<div class=\"http-example-label\">Request Message</div>\n" + 
	    escapetext(request);
	results_div.appendChild(request_box);
    }

    var result_box;
    if (result_format == "tree") {
	result_box = document.createElement("div");
	result_box.className = "response-tree";
	result_box.innerHTML = 
	    "Waiting for response from Serif...";
    } else {
	result_box = document.createElement("pre");
	result_box.className = "example-response";
	result_box.innerHTML = 
	    "<div class=\"http-example-label\">Serif Response</div>\n" + 
	    "Waiting for response from Serif...";
    }
    results_div.appendChild(result_box);

    $.ajax({
	type: 'POST',
	url: "SerifXMLRequest",
	data: request,
	success: function(xml, textStatus, XMLHttpRequest) {  
	    if (result_format == "tree") {
		result_id = "result-tree"
		result_box.innerHTML = xml_tree(xml.documentElement, 
						result_id);
		expandToDepth(result_id,4);
		processList(document.getElementById(result_id));
	    } else {
		result_box.innerHTML = 
		    "<div class=\"http-example-label\">" + 
		    "Serif Response</div>" + 
		    ppxml(xml.documentElement);
	    }
	},
	error: function(XMLHttpRequest, textStatus, errorThrown) {
	    if (XMLHttpRequest.status == 0) {
		results_div.innerHTML =
		    "<h3>Unable to contact the Serif server.</h3>";
	    } else if (XMLHttpRequest.status == 200) {
		results_div.innerHTML = 
		    "<h3>Error: Serif returned OK (200) but no data</h3>";
	    } else {
		results_div.innerHTML = 
		    "<h3> Error (" + XMLHttpRequest.status + "): " +
		    XMLHttpRequest.statusText + "</h3>\n" +
		    "<pre class=\"example-response-error\">"+
		    escapetext(XMLHttpRequest.responseText)+
		    "</pre>";
	    }
	}
    });
}

function escapetext(s) {
    return s
	.replace(/&/g, "&amp;")
	.replace(/</g, "&lt;")
	.replace(/>/g, "&gt;");
}

function escapehtml(s) {
    return s
	.replace(/&/g, "&amp;amp;")
	.replace(/</g, "&amp;lt;")
	.replace(/>/g, "&amp;gt;");
}

function xml_tree(elt, id) {
    return (
	"[<a href=\"#\" onClick=\"javascript: expandTree('"+
	    id+"'); return false\">" +
	    "Expand Tree</a>] " +
	    "[<a href=\"#\" onClick=\"javascript: collapseTree('"+
	    id+"'); expandToDepth('"+id+"',3); return false;\">" +
	    "Collapse Tree</a>] " +
	    "<ul class=\"mktree\" id=\""+id+"\">\n"+
	    xml_tree_helper(elt)+"</ul>\n");
}

function get_elt_id(elt) {
    for (var i=0; i<elt.attributes.length; i++) {
	if (elt.attributes[i].name.toLowerCase() == "id") {
	    return elt.attributes[i].value;
	}
    }
    return null;
}

function showPointer(target_id) {
    var target = document.getElementById("result-tree-"+target_id);
    if (target == null) {
	// Pointers to SynNodes don't actually have a target element
	// if we're using treebank parse representations; so just
	// highlight the whole parse tree instead.
	var pos = target_id.search(/.\d+$/);
	if (pos > 0)
	    showPointer(target_id.substring(0, pos));
	return;
    }
    expandToItem("result-tree", "result-tree-"+target_id);
    blink("result-tree-"+target_id);
}

function blink(elt_id) {
    var elt = document.getElementById(elt_id);
    elt.style.background = "#fcc";
    setTimeout("clear_blink(\""+elt_id+"\")", 500);
}

function clear_blink(elt_id) {
    var elt = document.getElementById(elt_id);
    elt.style.background = "";
}

function linkIdRefs(attrval) {
    var ids = attrval.split(" ");
    s = "";
    for (var i=0; i<ids.length; i++) {
	if (i > 0) s += " ";
	s += "<a href=\"#" + ids[i] + 
	    "\" onClick=\"javascript:showPointer('"+
	    ids[i]+"'); return false;\">" + 
	    escapehtml(ids[i]) + "</a>";
    }
    return s;
}
    
function xml_tree_helper(elt) {
    if (elt.nodeType == 8) { // 8 = COMMENT_NODE
	return "<pre class=\"tree-comment\">&lt;!-- "+
	    escapehtml(elt.nodeValue)+" --&gt;" + "</pre>\n";
    }

    // Check if this is a text element.
    if (!elt.tagName) { 
	if (elt.nodeValue.replace(/\s/g,"") == "") {
	    return "";
	} else {
	    return "<pre class=\"tree-leaf\">" + 
		escapehtml(elt.nodeValue) + "</pre>\n"; 
	}
    }
    // Display the XML element with a little syntax higlighting.
    var s = "";
    var elt_id = get_elt_id(elt);
    if (elt_id == null)
	s += "<li>";
    else
	s += "<li id=\"result-tree-"+elt_id+"\">";
    s += "<span class=\"xml-elt\">&lt;<span class=\"xml-tag\">" + 
	escapehtml(elt.tagName) + "</span>\n";
    for (var i=0; i<elt.attributes.length; i++) {
	var attrname = elt.attributes[i].name;
	var attrval = elt.attributes[i].value;
	var is_idref = ((attrname.toLowerCase() != "id") && 
	                ((attrname.substr(attrname.length-2, 2).toLowerCase()=="id") ||
			 (attrname.substr(attrname.length-3, 3).toLowerCase()=="ids") ||
			 (attrname=="start_token") || (attrname=="end_token")));
	s += " <span class=\"xml-attr\">" +
	    escapehtml(attrname) + "</span>=\"" +
	    "<span class=\"xml-attrval\">";

	if (is_idref)
	    s += linkIdRefs(attrval);
	else
	    s += escapehtml(attrval);
	s += "</span>\"";
    }
    if (elt.childNodes.length == 0) {
	s += "/&gt;</span>\n";
    } else {
	s += "&gt;</span>\n";
	s += "<ul>";
	for (var i=0; i<elt.childNodes.length; i++) {
	    s += xml_tree_helper(elt.childNodes[i]);
	}
	s += "</ul></li>";
    }
    return s;
    
}

function ppxml(elt) {
    if (elt.nodeType == 8) { // 8 = COMMENT_NODE
	return "<span class=\"xml-comment\">&lt;!-- " + 
	escapehtml(elt.nodeValue) + " --&gt;</span>"; 
    }
    // Check if this is a text element.
    if (!elt.tagName) { 
	return "<span class=\"xml-text\">" + 
	escapehtml(elt.nodeValue) + "</span>"; 
    }
    // Display the XML element with a little syntax higlighting.
    var s = "<span class=\"xml-elt\">&lt;<span class=\"xml-tag\">" + 
	escapehtml(elt.tagName) + "</span>";
    for (var i=0; i<elt.attributes.length; i++) {
	s += " " + escapehtml(elt.attributes[i].name) +
	    "=\"<i>" + escapehtml(elt.attributes[i].value) + "</i>\"";
    }
    if (elt.childNodes.length == 0) {
	s += "/&gt;</span>";
    } else {
	s += "&gt;</span>";
	for (var i=0; i<elt.childNodes.length; i++) {
	    s += ppxml(elt.childNodes[i]);
	}
	s += "<span class=\"xml-elt\">&lt;/<span class=\"xml-tag\">" +  
	    escapehtml(elt.tagName) + "&gt;</span></span>"
    }
    return s;
}
