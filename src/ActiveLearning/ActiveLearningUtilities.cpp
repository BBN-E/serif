#include "Generic/common/leak_detection.h"
#include "ActiveLearningUtilities.h"

#include <sstream>
#include <boost/lexical_cast.hpp>
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"

#ifndef WIN32
#include <unistd.h>
#endif

using std::string;
using std::stringstream;
using boost::lexical_cast;

#ifndef WIN32
string findMachineName() {
	char hostname[1024];
	hostname[1023] = '\0';
	gethostname(hostname, 1023);

	return string(hostname);
}
#endif

void ActiveLearningUtilities::sendUIReadyNotification(const string& emailAddress, 
													  unsigned int port) 
{
#ifndef WIN32
	stringstream url_stream;
	url_stream << "http://" << findMachineName() << ":" << port;
	string url = url_stream.str();
	string annotator = ParamReader::getRequiredParam("annotator");
	cout << "Sending e-mail notification to annotator " << annotator << endl;
	stringstream command;
	command << ParamReader::getParam("mail_command") << " -s 'LearnIt2 Annotation Request' " << annotator;
	FILE *mailer = popen(command.str().c_str(), "w");
	stringstream msg;
	msg << "Hello,\nPlease use your web browser to annotation features and instances requested by LearnIt2:\n"
		<< "<a href='" << url << "'>" << url << "'</a>'";
	fprintf(mailer, "%s", msg.str().c_str());
	pclose(mailer);
#else
	SessionLogger::warn("active_learning_wrong_architecture") << 
		"Cannot send e-mail notifications when compiled for Windows." << std::endl;
#endif

}

bool ActiveLearningUtilities::runActiveLearning(const string& documentRoot, 
				boost::thread& optimizationThread, 
				mg_callback_t event_handler) 
{
	bool server_success = false;

	// Mongoose needs the random number generator
	srand((unsigned) time(0));
	SessionLogger::info("start_server") << "Starting active learning server" ;
	SessionLogger::info("start_server") << "Serving from " << documentRoot;

	for (int port = 8081; port <= 8099; ++port) {
		string portString = lexical_cast<string>(port);
		const char* options[] = { 
			"document_root", documentRoot.c_str(), 
			"listening_ports", portString.c_str(),
			"num_threads", "1" , NULL
		};
		struct mg_context* ctx = mg_start(event_handler, NULL, options);
		if (ctx == NULL) {
			SessionLogger::warn("active_learning_server_start_failed") 
				<< "Failed to start active learning web server on port "
				<< port;
			continue;
		}

		sendUIReadyNotification(ParamReader::getRequiredParam("annotator"), port);

		optimizationThread.join();
		mg_stop(ctx);
		server_success = true;
		break;
	}

	return server_success;
}
