#ifndef _GRAPH_DUMPER_H_
#define _GRAPH_DUMPER_H_

#include <string>
#include <vector>
#include <iostream>
#include <boost/filesystem.hpp>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"

using boost::filesystem::path;
using boost::filesystem::copy_option;

template <typename GraphType>
class GraphDumper {
	public:
		typedef std::vector<boost::shared_ptr<GraphType> > Graphs;

		GraphDumper(const std::wstring& title, const std::string& directory, 
				const std::string& filename) 
			: out((directory + filename+".html").c_str()) 
		{
			std::string css = ParamReader::getRequiredParam("graph_dumper_css");
			std::string output_css = directory + filename + ".css";

			if (boost::filesystem::exists(output_css)) {
				boost::filesystem::remove(output_css);
			}
			boost::filesystem::copy_file(path(css), path(output_css), 
					copy_option::overwrite_if_exists);

			std::wstring w_css = UnicodeUtil::toUTF16StdString(output_css);
			std::wstring qtipDir = UnicodeUtil::toUTF16StdString(
					ParamReader::getRequiredParam("qtip_dir"));

			out << L"<html>\n\t<head>" << std::endl;
			out << L"\t\t<title>" << title << L"</title>" << std::endl;
			out << L"\t\t<link rel = 'stylesheet' type = 'text/css' "
				<< "href='" << w_css << "'>" << std::endl;
			out << L"\t\t<link rel = 'stylesheet' type = 'text/css' "
				<< "href='" << qtipDir << "\\jquery.qtip.css'>" << std::endl;
			out << L"\t\t<script type ='text/javascript' src='"
				<< L"https://ajax.googleapis.com/ajax/libs/jquery/1.7.1/jquery.min.js'></script>" << std::endl;
			out << L"\t\t<script type = 'text/javascript' src='" << qtipDir 
				<< L"\\jquery.qtip.js'></script>" << std::endl;
			//{show:{event:'click'}});</script>" << std::endl;
			out << L"\t</head>" << std::endl;
			out << L"\t<body>" << std::endl;
		}

		~GraphDumper() {
			out << L"\t\t<script>$('span[title]').click(function(e) { e.stopPropagation(); });</script>" << std::endl;
			out << L"\t\t<script>$('[title]').qtip({show: {event: 'click'}, position: {viewport: true}, hide: {event: 'unfocus'}})</script>" << std::endl;
			out << L"\t</body>" << std::endl;
			out << L"</html>" << std::endl;
		}

		void dumpAll(const Graphs& graphs)
		{
			for(typename Graphs::const_iterator it = graphs.begin(); 
					it!=graphs.end(); ++it)
			{
				dump(**it);
			}
		}

		static const std::wstring& color(unsigned int i) {
			initColors(); 
			return _colors[i];
		}

		static size_t nColors() { 
			initColors(); 
			return _colors.size(); 
		}
	protected:
		std::wofstream out;
		static std::vector<std::wstring> _colors;
		static bool colors_init;

		virtual void dump(const GraphType& graph) = 0;

		static void initColors() {
			if (!colors_init) {
				colors_init = true;
	
				_colors.push_back(L"#FF0000");
				_colors.push_back(L"#00FF00");
				_colors.push_back(L"#0000FF");
				_colors.push_back(L"#CC9966");
				_colors.push_back(L"#99ffFF");
				_colors.push_back(L"#FFCC33");
				_colors.push_back(L"#FF99999");
				_colors.push_back(L"#00CCFF");
				_colors.push_back(L"#FF3399");
				_colors.push_back(L"#CCCCCC");
				_colors.push_back(L"#FFFF00");
			} 
		}

};

template <typename GraphType>
bool GraphDumper<GraphType>::colors_init = false;
template <typename GraphType>
std::vector<std::wstring > GraphDumper<GraphType>::_colors;

template <typename GraphType>
class ModelDumper {
	public:
		ModelDumper(const std::string& directory, const std::string& filename)
			: out((directory + filename+".html").c_str()) 
		{
			std::string css = ParamReader::getRequiredParam("model_dumper_css");
			std::string output_css = directory + filename + ".css";

			if (boost::filesystem::exists(output_css)) {
				boost::filesystem::remove(output_css);
			}
			boost::filesystem::copy_file(path(css), path(output_css), 
					copy_option::overwrite_if_exists);

			std::wstring w_css = UnicodeUtil::toUTF16StdString(output_css);

			out << L"<html>\n\t<head>" << std::endl;
			out << L"\t\t<title>Dumped Model</title>" << std::endl;
			out << L"\t\t<link rel = 'stylesheet' type = 'text/css' "
				<< "href='" << w_css << "'>" << std::endl;
			out << L"\t</head>" << std::endl;
			out << L"\t<body>" << std::endl;
		}

		~ModelDumper() {
			out << L"\t</body>" << std::endl;
			out << L"</html>" << std::endl;
		}

		wostream& stream() {
			return out;
		}
private:
		std::wofstream out;
};

#endif

