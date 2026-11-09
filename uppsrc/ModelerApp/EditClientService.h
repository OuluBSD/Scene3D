#ifndef _ModelerApp_EditClientService_h_
#define _ModelerApp_EditClientService_h_
#if 0


class EditClientService : public DaemonService {
	EnetServiceClient* client = 0;
	bool is_calling = false;
	
public:
	Edit3D* edit = 0;
	
	// Remote connection
	RemoteExchange sync;
	bool debug = false;
	bool ready = false;
	
public:
	bool Init(String name) override;
	void Update() override;
	void Stop() override;
	void Deinit() override;
	
	void SetReady(bool b=true) {ready = b;}
	void SetDebuggingMode() {debug = true;}
	
};

#endif
#endif
