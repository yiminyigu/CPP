/////////////////////////////////////////////////////////////////////////
// ServerPrototype.cpp - Console App that processes incoming messages  //
// ver 1.0                                                             //
// Jim Fawcett, CSE687 - Object Oriented Design, Spring 2018           //
/////////////////////////////////////////////////////////////////////////

#include "ServerPrototype.h"
#include <chrono>
#include <functional>
#include "../CodePubr/CodePubrFactory.h"
#include "../Utilities/CodeUtilities/CodeUtilities.h"
#include "../customLogger/customLogger.h"
#include "../FileSystem/FileSystem.h"
#include "../customLogger/customLogger.h"
#include "../ThreadPool/ThreadPool.h"
namespace MsgPassComm = MsgPassingCommunication;

using namespace MsgPassComm;
using namespace Repository;
using namespace FileSystem;
using namespace CodePublisher;
using namespace Utilities;
using Msg = MsgPassingCommunication::Message;

//----< initialize server endpoint and give server a name >----------

inline ProtoServer::ProtoServer(MsgPassingCommunication::EndPoint ep, const std::string& name)
	: comm_(ep, name) {}

//----< start server's instance of Comm >----------------------------

inline void ProtoServer::start()
{
	comm_.start();
}
//----< stop Comm instance >-----------------------------------------

inline void ProtoServer::stop()
{
	if (msgProcThrd_.joinable())
		msgProcThrd_.join();
	comm_.stop();
}
//----< pass message to Comm for sending >---------------------------

inline void ProtoServer::postMessage(MsgPassingCommunication::Message msg)
{
	comm_.postMessage(msg);
}
//----< get message from Comm >--------------------------------------

inline MsgPassingCommunication::Message ProtoServer::getMessage()
{
	Msg msg = comm_.getMessage();
	return msg;
}
//----< add ServerProc callable object to server's dispatcher >------

inline void ProtoServer::addMsgProc(Key key, ServerProc proc)
{
	dispatcher_[key] = proc;
}
void Repository::ProtoServer::assembleDispatcher()
{
	//ServerProc sp = [&](MsgPassingCommunication::Message msg) { return echo(msg); };
	addMsgProc("echo", std::bind(&ProtoServer::echo, this, std::placeholders::_1));
	addMsgProc("quit", std::bind(&ProtoServer::quit, this, std::placeholders::_1));
	addMsgProc("serverQuit", std::bind(&ProtoServer::serverQuit, this, std::placeholders::_1));
}
//----< start processing messages on child thread >------------------

inline void ProtoServer::processMessages()
{
	auto proc = [&]()
	{
		if (dispatcher_.size() == 0)
		{
			customLogging::StaticLogger::write(logOn, "\n  no server procs to call");
			return;
		}
		ThreadPool<5> trpl;
		running_ = true;
		while (running_)
		{
			Msg msg = getMessage();
			customLogging::StaticLogger::write(logOn, "\n Server received message: " + msg.command() + " from " + msg.from().toString());
			msg.show();
			if (msg.to().toString() == msg.from().toString()) {  // avoid infinite message loop
				customLogging::StaticLogger::write(logOn, "\n  server attempting to post to self");
				continue;
			}
			CallWrapper cw(dispatcher_[msg.command()], msg);
			trpl.workItem(cw);
		}
		customLogging::StaticLogger::write(logOn, "\n  server message processing thread is shutting down");
	};
	std::thread t(proc);
	//SetThreadPriority(t.native_handle(), THREAD_PRIORITY_HIGHEST);
	customLogging::StaticLogger::write(logOn, "\n  starting server thread to process messages");
	msgProcThrd_ = std::move(t);
}


bool Repository::ProtoServer::serverQuit(MsgPassingCommunication::Message msg)
{
	running_ = false;
	return false;
}


Files ProtoServer::getFiles(const Repository::SearchPath& path)
{
	return Directory::getFiles(path);
}

Dirs ProtoServer::getDirs(const Repository::SearchPath& path)
{
	return Directory::getDirectories(path);
}

template<typename T>
void show(const T& t, const std::string& msg)
{
	std::cout << "\n  " << msg.c_str();
	for (auto item : t)
	{
		std::cout << "\n    " << item.c_str();
	}
}

//using inhertence is a better choice for future
bool Repository::ProtoServer::echo(MsgPassingCommunication::Message msg) {
	Msg reply = msg;
	reply.to(msg.from());
	reply.from(msg.to());
	postMessage(reply);
	customLogging::StaticLogger::write(logOn, "\n Server reply message: " + reply.command() + " to " + reply.to().toString());
	reply.show();
	return true;
};
/////

void Repository::CodePubrServer::assembleDispatcher()
{
	ProtoServer::assembleDispatcher();
	addMsgProc("demoOn", std::bind(&CodePubrServer::demoOn, this, std::placeholders::_1));//demoOn is for demo opration, not control of logger
	addMsgProc("demoOff", std::bind(&CodePubrServer::demoOff, this, std::placeholders::_1));

	addMsgProc("getFiles", std::bind(&CodePubrServer::getFiles, this, std::placeholders::_1));
	addMsgProc("getDirs", std::bind(&CodePubrServer::getDirs, this, std::placeholders::_1));
	addMsgProc("publish", std::bind(&CodePubrServer::fullyProduce, this, std::placeholders::_1));
	addMsgProc("downloadSelected", std::bind(&CodePubrServer::downloadSelected, this, std::placeholders::_1));
	addMsgProc("clearAll", std::bind(&CodePubrServer::deleteClientStore, this, std::placeholders::_1));
}

bool Repository::CodePubrServer::demoOn(MsgPassingCommunication::Message msg)
{
	::demoOn = true;
	return true;
}

bool Repository::CodePubrServer::demoOff(MsgPassingCommunication::Message msg)
{
	::demoOn = false;
	return true;
}

void CodePubrServer::addandSendAssistEntities(Msg& reply) {
	std::string cssPath = Path::getFullFileSpec(ClientsDataRoot + "/css");
	Files csses = Directory::getFiles(cssPath);
	std::string jsesPath = Path::getFullFileSpec(ClientsDataRoot + "/js");
	Files jses = Directory::getFiles(jsesPath);
	std::string notFoundsPath = Path::getFullFileSpec(ClientsDataRoot + "/notFound");
	Files notFoundes = Directory::getFiles(notFoundsPath);
	for (int i = 0; i < csses.size(); i++) {
		reply.attribute("download-css" + std::to_string(i), csses[i]);
	}
	for (int i = 0; i < jses.size(); i++) {
		reply.attribute("download-js" + std::to_string(i), jses[i]);
	}
	for (int i = 0; i < notFoundes.size(); i++) {
		reply.attribute("download-notFound" + std::to_string(i), notFoundes[i]);
	}

	for (auto filename : csses) {
		reply.file(cssPath + "\\" + filename);
		reply.attributes()["saveFilePath"] = Path::getFullFileSpec(reply.value("saveFilePath") + "\\..\\css");
		postMessage(reply);
		reply.show();
	}
	for (auto filename : jses) {
		reply.file(jsesPath + "\\" + filename);
		reply.attributes()["saveFilePath"] = Path::getFullFileSpec(reply.value("saveFilePath") + "\\..\\js");
		postMessage(reply);
		reply.show();
	}
	for (auto filename : notFoundes) {
		reply.file(notFoundsPath + "\\" + filename);
		reply.attributes()["saveFilePath"] = Path::getFullFileSpec(reply.value("saveFilePath") + "\\..\\notFounds");
		postMessage(reply);
		reply.show();
	}
	reply.attributes()["saveFilePath"] = Path::getFullFileSpec(reply.value("saveFilePath") + "\\..\\HTML");
}

bool Repository::CodePubrServer::getFiles(MsgPassingCommunication::Message msg) {
  Msg reply;
  reply.to(msg.from());
  reply.from(msg.to());
  reply.command("getFiles");
  std::string path = msg.value("path");
  if (path != "")
  {
    std::string searchPath = projectsStoreRoot;
    if (path != ".")
      searchPath = searchPath + "\\" + path;
    Files files = ProtoServer::getFiles(searchPath);
    size_t count = 0;
    for (auto item : files)
    {
      std::string countStr = Utilities::Converter<size_t>::toString(++count);
      reply.attribute("file" + countStr, item);
    }
  }
  else
  {
    std::cout << "\n  getFiles message did not define a path attribute";
  }
  postMessage(reply);
  customLogging::StaticLogger::write(logOn, "\n Server reply message: " + reply.command() + " to " + reply.to().toString());
  reply.show();
  return true;
};

bool Repository::CodePubrServer::getDirs(MsgPassingCommunication::Message msg) {
  Msg reply;
  reply.to(msg.from());
  reply.from(msg.to());
  reply.command("getDirs");
  std::string path = msg.value("path");
  if (path != "")
  {
    std::string searchPath = projectsStoreRoot;
    if (path != ".")
      searchPath = searchPath + "\\" + path;
	searchPath = Path::getFullFileSpec(searchPath);
    Files dirs = ProtoServer::getDirs(searchPath);
    size_t count = 0;
    for (auto item : dirs)
    {
      if (item != ".." && item != ".")
      {
        std::string countStr = Utilities::Converter<size_t>::toString(++count);
        reply.attribute("dir" + countStr, item);
      }
    }
  }
  else
  {
    std::cout << "\n  getDirs message did not define a path attribute";
  }
  postMessage(reply);
  customLogging::StaticLogger::write(logOn, "\n Server reply message: " + reply.command() + " to " + reply.to().toString());
  reply.show();
  return true;
};

bool Repository::CodePubrServer::fullyProduce(MsgPassingCommunication::Message msg) {
	std::vector<std::string> args;
	args.push_back(".exe");
	args.push_back(Path::getFullFileSpec(Path::fileSpec(projectsStoreRoot, msg.value("path"))));
	args.push_back("/s");
	args.push_back("*.h");
	args.push_back("*.cpp");
	int argc = args.size();
	char** argv = new char*[argc];
	for (int i = 0; i < argc; i++) {		
		auto tempc = args[i].c_str();
		size_t len = strlen(tempc) + 1;
		argv[i] = new char[len];
		strcpy_s(argv[i], len, tempc);
	}
	auto srcEp = EndPoint::fromString(msg.value("from"));
	std::string clientstorePath = Path::fileSpec(ClientsDataRoot, srcEp.address + std::to_string(srcEp.port));
	clientstorePath = Path::getFullFileSpec(clientstorePath);
	if (Directory::exists(clientstorePath)) {
		for (auto file : Directory::getFiles(clientstorePath))
			bool b = FileSystem::File::remove(Path::fileSpec(clientstorePath,file));
		Directory::remove(clientstorePath);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	bool b = Directory::create(clientstorePath);
	ProcessCmdLine pcl(argc, argv);
	CodePubrFactory cpf;
	ICodePubr* pIcp = cpf.CreateCustom(&pcl, clientstorePath);
	auto products = pIcp->ProtoFullyProduce();
	Msg reply;
	reply.to(msg.from());
	reply.from(msg.to());
	reply.command("publish");
	int count = 1;
	for (auto product : products)
	{
		reply.attribute("product" + std::to_string(count), Path::getName(product));
		count++;
		/*if (::demoOn&&count == 6)
			break;*/
	}
	delete[] argv;
	delete pIcp;
	postMessage(reply);
	customLogging::StaticLogger::write(logOn, "\n Server reply message: " + reply.command() + " to " + reply.to().toString());
	reply.show();
	return true;
};

bool Repository::CodePubrServer::downloadSelected(MsgPassingCommunication::Message msg) {
	Msg reply=msg;
	reply.to(msg.from());
	reply.from(msg.to());
	addandSendAssistEntities(reply);
	auto srcEp = EndPoint::fromString(msg.value("from"));
	std::string srcstorePath = Path::fileSpec(ClientsDataRoot, srcEp.address + std::to_string(srcEp.port));
	//important to track original msg cuz reply contains assist file entities
	for (auto entity : msg.attributes()) {
		if (entity.first.find("download") != std::string::npos) {
			std::string fFileSpec = Path::getFullFileSpec(srcstorePath + "\\" + entity.second);
			reply.file(fFileSpec);
			postMessage(reply);
			customLogging::StaticLogger::write(logOn, "\n Server reply message: " + reply.command() + " to " + reply.to().toString());
			reply.show();
		}
	}
	return true;
};

bool Repository::CodePubrServer::deleteClientStore(MsgPassingCommunication::Message msg) {
	auto srcEp = EndPoint::fromString(msg.value("from"));
	std::string clientstorePath = Path::fileSpec(ClientsDataRoot, srcEp.address + std::to_string(srcEp.port));
	if (Directory::exists(clientstorePath)) {
		for (auto file : Directory::getFiles(clientstorePath))
			bool b = FileSystem::File::remove(Path::fileSpec(clientstorePath, file));
		Directory::remove(clientstorePath);
	}
	Msg reply;
	reply.to(msg.from());
	reply.from(msg.to());
	reply.command("clearAll");

	return true;

};


#ifdef TEST_SERVER
using namespace customLogging;
int main()
{
	customLogging::StaticLogger::attach(&std::cout);
	customLogging::StaticLogger::start();
	customLogging::StaticLogger::write(true, "\n  Server Initiating...");
	customLogging::StaticLogger::write(true, "\n ==========================\n");

	CodePubrServer server(serverEndPoint, "CodePubrServer");
	server.assembleDispatcher();
	server.start();

	EndPoint clientEP1("localhost", 8082);
	EndPoint clientEP2("localhost", 8083);

	server.processMessages();
	Comm client(clientEP1, "myClient");
	client.start();

	Msg msg(serverEndPoint,clientEP1);  // send to self
	msg.name("test1");
	msg.command("publish");
	msg.attributes()["path"] = "./Project1";
	client.postMessage(msg);

	msg.from(clientEP2);
	client.postMessage(msg);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));


	//Msg msg(serverEndPoint, clientEP);  // send to self
	//msg.command("downloadSelected");
	//msg.attribute("saveFilePath", Path::getFullFileSpec("..\\ClientStorage\\localhost8082\\HTML"));
	//msg.attribute("download-product1", "AbstrSynTree.cpp.html");
	//msg.attribute("download-product2", "CodePubr.h.html");
	//msg.attribute("download-product3", "Converter.h.html");
	//client.postMessage(msg);
	std::cout << "\n  press enter to exit";
	std::cin.get();
	std::cout << "\n";

	msg.from(serverEndPoint);
	msg.command("serverQuit");
	server.postMessage(msg);
	server.stop();
	customLogging::StaticLogger::stop();
	return 0;
}
#endif // TEST_SERVER
using namespace customLogging;
int main()
{
	customLogging::StaticLogger::attach(&std::cout);
	customLogging::StaticLogger::start();
	customLogging::StaticLogger::write(true, "\n  Server Initiating...");
	customLogging::StaticLogger::write(true, "\n ==========================\n");

	CodePubrServer server(serverEndPoint, "CodePubrServer");
	server.assembleDispatcher();
	server.start();

	//EndPoint clientEP1("localhost", 8082);
	//EndPoint clientEP2("localhost", 8083);

	//Comm client(clientEP1, "myClient");
	//client.start();
	server.processMessages();

	//Msg msg(serverEndPoint,clientEP1);  // send to self
	//msg.name("test1");
	//msg.command("publish");
	//msg.attributes()["path"] = "./Project3_Zhengqi_Cai";
	//client.postMessage(msg);

	//msg.from(clientEP2);
	//client.postMessage(msg);
	//std::this_thread::sleep_for(std::chrono::milliseconds(1000));


	//Msg msg(serverEndPoint, clientEP);  // send to self
	//msg.command("downloadSelected");
	//msg.attribute("saveFilePath", Path::getFullFileSpec("..\\ClientStorage\\localhost8082\\HTML"));
	//msg.attribute("download-product1", "AbstrSynTree.cpp.html");
	//msg.attribute("download-product2", "CodePubr.h.html");
	//msg.attribute("download-product3", "Converter.h.html");
	//client.postMessage(msg);
	std::cout << "\n  press enter to exit";
	std::cin.get();
	std::cout << "\n";

	Msg msg(serverEndPoint,serverEndPoint);
	msg.command("serverQuit");
	server.postMessage(msg);
	server.stop();
	customLogging::StaticLogger::stop();
	return 0;
}
