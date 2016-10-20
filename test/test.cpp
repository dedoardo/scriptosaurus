#define scriptosaurus_win32
#define scriptosaurus_live
#define scriptosaurus_msvc "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin"
#include "scriptosaurus.hpp"

struct SampleScript : public scriptosaurus::Script
{
	SampleScript() : Script("SampleScript") { } 

	scriptosaurus_routine(sample_callback);
};

int main()
{
	using namespace scriptosaurus;
	ScriptDaemon daemon("scripts");
	SampleScript s1;
	daemon.add(s1, { scriptosaurus_prepare(s1, sample_callback) });

	daemon.link_all();
	
	while (true) 
		scriptosaurus_call(s1, sample_callback, nullptr);
}