#include "hook.h"

void MessageHandler(SKSE::MessagingInterface::Message *a_msg)
{
	switch (a_msg->type)
	{
	case SKSE::MessagingInterface::kDataLoaded:

		hooks::DGD::install();
		hooks::Settings::GetSingleton()->Load();
		hooks::Settings::GetSingleton()->general.LoadSettings();

		break;

	default:

		break;
	}
}

void Init()
{
	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener("SKSE", MessageHandler);
}

void Load(){
	// hooks::DGD::install_protected();
}