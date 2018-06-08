#include <windows.h>
#include <cmath>
#include <winsock.h>
#include <stdio.h>
#include <fstream>
#include <process.h>
#include "sqlite3.h"
#include <ctime>
#include <string>
#include <sstream>
#include <list>

HWND h1, h2, h3;
HINSTANCE hIns;
#define IDZAMKNIJ 425
#define IDEDIT 426
#define IDEDIT2 427
#define IDSIEC 428
#define DEFAULT_BUFLEN 512
#define IDT_TIMER1 1
#define IDPOMIAR 424
#define IDSTOPPOMIAR 524
#define IDRYSUJ 429

namespace patch
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}

CRITICAL_SECTION CriticalSection;
std::string getTemperature(char*, char*);
int InitWinsock();
int status = 0;

char tBuff [200];
bool pomiar = false; //to zwraca nam w1tek

//do tych list zaciaga dane z bazy (1 kolumna - 1 lista)
std::list<std::string> dateList;
std::list<std::string> timeList;
std::list<std::string> cityList;
std::list<std::string> tempList;

// *** BAZA ***

bool CreateDatabase() {
	
	sqlite3 *db;
	sqlite3_stmt *statement;
	char *errorMessage = 0;
	int stepNotComplete;
	std::string query = "CREATE TABLE IF NOT EXISTS Weather (date TEXT, time TEXT, city TEXT, temperature INTEGER);";
	FILE *outStream  = stdout;
	
	stepNotComplete = sqlite3_open("weather.sqlite3", &db);
	
	if (stepNotComplete) {
		fprintf(outStream, "Nie mozna otworzyc bazy danych - %s\n", sqlite3_errmsg(db));
		return 1;	
	}

	stepNotComplete = sqlite3_exec(db, query.c_str(), NULL, NULL, &errorMessage);

	if (stepNotComplete) {
		fprintf(outStream, "%s\n", sqlite3_errmsg(db));
		return 1;	
	}

	sqlite3_close(db);
	
	return 0;
}

bool DatabaseWriter(std::string temp, std::string city) {
	
	time_t now;
	struct tm timeinfo;
	time(&now);	
	localtime_r(&now, &timeinfo);

	timeinfo.tm_year += 1900;
	timeinfo.tm_mon += 1;
	
	std::string dateNow = patch::to_string(timeinfo.tm_mday);
	dateNow += "-";
	dateNow += patch::to_string(timeinfo.tm_mon);
	dateNow += "-";
	dateNow += patch::to_string(timeinfo.tm_year);
	
	std::string timeNow = patch::to_string(timeinfo.tm_hour);
	timeNow += ":";
	timeNow += patch::to_string(timeinfo.tm_min);
	
	sqlite3 *db;
	sqlite3_stmt *statement;
	char *errorMessage = 0;
	int stepNotComplete;
	
	std::string query = "INSERT INTO Weather (date, time, city, temperature) VALUES ('";
	query += dateNow;
	query += "', '";
	query += timeNow;
	query += "', '";
	query += city;
	query += "', ";
	query += temp;
	query += ");";
	
	FILE *outStream  = stdout;
	
	stepNotComplete = sqlite3_open("weather.sqlite3", &db);
	
	if (stepNotComplete) {
		fprintf(outStream, "Nie mozna otworzyc bazy danych - %s\n", sqlite3_errmsg(db));
		return 1;	
	}
	
	stepNotComplete = sqlite3_exec(db, query.c_str(), NULL, NULL, &errorMessage);

	if (stepNotComplete) {
		fprintf(outStream, "%s\n", sqlite3_errmsg(db));
		return 1;	
	}

	sqlite3_close(db);	
	
	/*
	std::string ret = patch::to_string(timeinfo.tm_mday);
	ret += "-";
	ret += patch::to_string(timeinfo.tm_mon);
	ret += "-";
	ret += patch::to_string(timeinfo.tm_year);
	ret += " ";
	ret += patch::to_string(timeinfo.tm_hour);
	ret += ":";
	ret += patch::to_string(timeinfo.tm_min);
	*/
	
	return 0;
}

bool DatabaseReaderDate() {
		
	sqlite3 *db;
	sqlite3_open("weather.sqlite3", &db);
	
	sqlite3_stmt *statement;
	sqlite3_prepare_v2(db, "SELECT date FROM Weather;", -1, &statement, NULL); 
	
	while (sqlite3_step(statement) == SQLITE_ROW) {
		std::string dateString = std::string((char*)sqlite3_column_text(statement, 0));
		dateList.push_back(dateString);
	}

	sqlite3_finalize(statement);
	sqlite3_close(db);    
}

bool DatabaseReaderTime() {
		
	sqlite3 *db;
	sqlite3_open("weather.sqlite3", &db);
	
	sqlite3_stmt *statement;
	sqlite3_prepare_v2(db, "SELECT time FROM Weather;", -1, &statement, NULL); 
	
	while (sqlite3_step(statement) == SQLITE_ROW) {
		std::string timeString = std::string((char*)sqlite3_column_text(statement, 0));
		timeList.push_back(timeString);
	}

	sqlite3_finalize(statement);
	sqlite3_close(db);    
}

bool DatabaseReaderCity() {
		
	sqlite3 *db;
	sqlite3_open("weather.sqlite3", &db);
	
	sqlite3_stmt *statement;
	sqlite3_prepare_v2(db, "SELECT city FROM Weather;", -1, &statement, NULL); 
	
	while (sqlite3_step(statement) == SQLITE_ROW) {
		std::string cityString = std::string((char*)sqlite3_column_text(statement, 0));
		cityList.push_back(cityString);
	}

	sqlite3_finalize(statement);
	sqlite3_close(db);    
}

bool DatabaseReaderTemp() {
		
	sqlite3 *db;
	sqlite3_open("weather.sqlite3", &db);
	
	sqlite3_stmt *statement;
	sqlite3_prepare_v2(db, "SELECT temperature FROM Weather;", -1, &statement, NULL); 
	
	while (sqlite3_step(statement) == SQLITE_ROW) {
		std::string tempString = std::string((char*)sqlite3_column_text(statement, 0));
		tempList.push_back(tempString);
	}

	sqlite3_finalize(statement);
	sqlite3_close(db);    
}

// ** KONIEC BAZY ***

void FunThread(void*) { 
	//SPRAWDZENIE CZY JEST BAZA + TABELA
	//JESLI NIE MA, TWORZY Z AUTOMATU
    CreateDatabase();
    
	EnterCriticalSection(&CriticalSection); 
	InitWinsock();

	pomiar = true;
	while(pomiar)
	{
		std::string temperature = getTemperature("razniewski.eu", "5000");
	
		//MessageBox(h1, temperature.c_str() ,"Temperatura w czestochowie!",MB_OK);
	    // MessageBox(h1, (char*)IDSTOPPOMIAR ,"test",MB_OK);
	    //ZAPIS TEMPERATURY DO BAZY
		DatabaseWriter(temperature.c_str(), "Czestochowa");
		Sleep( 3000 );
	}
	
	
	WSACleanup();

	//ODCZYT Z BAZY, PODZIAL NA POSZCZEGOLNE KOLUMNY
	//DatabaseReaderDate();
	//DatabaseReaderCity(); 
	//DatabaseReaderTime();
	DatabaseReaderTemp();
	
	//dla sprawdzenia...
	//for( std::list<std::string>::iterator iter=tempList.begin(); iter != tempList.end(); iter++ )
    //  MessageBox(h1, (*iter).c_str(),"Temperatura w czestochowie!",MB_OK);
	
    LeaveCriticalSection(&CriticalSection);	

	_endthread();	
}

int InitWinsock () {
	WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        MessageBox(h1, "Nie moge za3adowaa Winsocka", "B31d!", MB_OK);
       // printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }                                    


    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
    }
    else {
    	char buff [200];
    	sprintf(buff, "Wersja Winsocka: %d.%d", LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));
		
		//MessageBox(h1, buff , "Gotowe!", MB_OK);
    	
        //printf("The Winsock 2.2 dll was found okay\n");
    }

   // WSACleanup();	
}

std::string getTemperature(char * host, char * port) {
	SOCKET ConnectSocket = INVALID_SOCKET;
	
	ConnectSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    if (ConnectSocket == INVALID_SOCKET) {
        char buff [300];
		sprintf(buff, "socket function failed with error = %d\n", WSAGetLastError());
		return NULL;
    }
    
	sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr(host);
    
    if (clientService.sin_addr.s_addr == INADDR_NONE) { //prawdopodobnie w formie DNS
    	struct hostent * h = gethostbyname(host);
    	if (h != NULL)
			clientService.sin_addr.s_addr=*((unsigned long*)h->h_addr);
		else {
			return NULL;
		}
    }
    
   	struct servent* se = getservbyname(port, "tcp");
    
    if (se != NULL) {
    	clientService.sin_port = se->s_port;
    }
    else {
    	if (atoi(port) == 0) {
    		//MessageBox(h1, "Wpisany port jest niepoprawny", "B31d!", MB_OK); 
    		return NULL;	
    	}
		else {
			if (atoi(port) < 1)
				return NULL;
			else
				clientService.sin_port = htons(atoi(port));
		}
	}
	
	
	char pbuff [2000];
    
    int iResult = connect(ConnectSocket, (sockaddr*)&clientService, sizeof(clientService));
    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }
    //wysylanie
	int recvbuflen = DEFAULT_BUFLEN;
    char recvbuf[DEFAULT_BUFLEN] = "";

    
 	int reciveResult = 0;

 	char sbuff[300];
 	
    char *sndbuf = (char*)"GET / HTTP/1.1\r\nHost: razniewski.eu\r\n\r\n";
    char rcvbuf[DEFAULT_BUFLEN] = "";
    
	
	int nTimeout = 1000; 
	setsockopt(ConnectSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&nTimeout, sizeof(int));
	
 	std::string leo;
    int sndResult = send( ConnectSocket, sndbuf, (int)strlen(sndbuf), 0 );
 	if (sndResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        WSACleanup();
        return NULL;
    }
 	int rciveResult = 0;
 	do {
     	rciveResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
     	char buffer[20];
		 _itoa(rciveResult, buffer, 20);
        if ( rciveResult > 0 ){
        	leo.append(recvbuf);
		}

    } while( rciveResult > 0 );
 	
 	std::string tempStart = "TEMPERATURASTART;";
 	std::string tempStop = ";STOPTEMPERATURA";
 	unsigned first = leo.find(tempStart);

 	
 	unsigned second = leo.find(";STOPTEMPERATURA");
	return leo.substr(first + tempStart.length() , second - first - tempStop.length() - 1);
}


void BudujGuziki(HWND hParent) {
	CreateWindowEx(0, "BUTTON", "Zamknij program", WS_VISIBLE|WS_CHILD,
		10, /* x */
		10, /* y */
		120, /* width */
		30, /* height */
		hParent, (HMENU)IDZAMKNIJ, hIns, NULL);	
		
		CreateWindowEx(0, "BUTTON", "Checkbox", WS_VISIBLE|WS_CHILD|BS_CHECKBOX,
		150, /* x */
		10, /* y */
		120, /* width */
		30, /* height */
		hParent, NULL, hIns, NULL);
		
		CreateWindowEx(0, "BUTTON", "AutoCheckbox", WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX,
		300, /* x */
		10, /* y */
		120, /* width */
		30, /* height */
		hParent, NULL, hIns, NULL);
		
		CreateWindowEx(0, "EDIT", "Edit", WS_VISIBLE|WS_CHILD|WS_BORDER,
		10, /* x */
		50, /* y */
		120, /* width */
		30, /* height */
		hParent, (HMENU)IDEDIT, hIns, NULL);	
		
		CreateWindowEx(0, "EDIT", "Edit2", WS_VISIBLE|WS_CHILD|WS_BORDER|ES_MULTILINE|ES_CENTER,
		10, /* x */
		100, /* y */
		300, /* width */
		300, /* height */
		hParent, (HMENU)IDEDIT2, hIns, NULL);
											

		CreateWindowEx(0, "BUTTON", "Uruchom pomiar", WS_VISIBLE|WS_CHILD,
		320, /* x */
		300, /* y */
		120, /* width */
		30, /* height */
		hParent, (HMENU)IDPOMIAR, hIns, NULL);
		
		CreateWindowEx(0, "BUTTON", "Zatrzymaj pomiar", WS_VISIBLE|WS_CHILD,
		450, /* x */
		300, /* y */
		120, /* width */
		30, /* height */
		hParent, (HMENU)IDSTOPPOMIAR, hIns, NULL);
		
		CreateWindowEx(0, "BUTTON", "Rysuj graf", WS_VISIBLE|WS_CHILD,
		400, /* x */
		350, /* y */
		120, /* width */
		30, /* height */
		hParent, (HMENU)IDRYSUJ, hIns, NULL);
		
}

/* This is where all the input to the window goes to */
LRESULT CALLBACK WndProc1(HWND h1, UINT Message, WPARAM wParam, LPARAM lParam) { //1
	
	HWND g1, e1, e2;
	
	switch(Message) {

	case WM_DESTROY: {
		PostQuitMessage(0);
		break;
	}

	case WM_TIMER: {
			//SetTimer(h1, IDT_TIMER1, 500, (TIMERPROC) NULL);
		if (wParam == IDT_TIMER1) {
			//MessageBox(h1,"Jest zdarzenie zegarowe","Uwaga",MB_OK);
			if (status > 0)
				if (status == 2) {
					status = 0;
					char trBuff [300];
					//sprintf(trBuff, "W1tek zakonczony. A obliczone z w1tku wynosi: %d", a);
					//MessageBox(h1, trBuff,"Uwaga!",MB_OK);	
				}
		}
	}

	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
			case IDZAMKNIJ: {
				/*MessageBox(h1,
	        		"Nacionieto przycisk zamknij",
	        		"Uwaga!",
	       			MB_ICONWARNING | MB_OK
		    	);*/
		    	g1 = (HWND)lParam;
		    	//EnableWindow(g1, FALSE);
		    	//ShowWindow(g1, SW_HIDE);
				//MoveWindow(g1, 200, 200, 200, 200, TRUE);
				//SetWindowText(g1, "Mucha");
				e1 = GetDlgItem(h1, IDEDIT);
				e2 = GetDlgItem(h1, IDEDIT2);
				//SetWindowText(e1, "Jab3ko");
				char buff1[10];
				char buff2[10];
				GetWindowText(e1, buff1, 10);
				GetWindowText(e2, buff2, 10);
				SetWindowText(e1, buff2);
				SetWindowText(e2, buff1);
				//delete buff1;
				//delete buff2;
				break;
			}
			
			case IDPOMIAR: {
				_beginthread(FunThread, 0, NULL);
				break;	
			}
			case IDSTOPPOMIAR: {
				pomiar = false;
				break;	
			}
			case IDRYSUJ: {
				//ShowWindow(h3,SW_SHOW);
			
				
				h3 = CreateWindowEx(WS_EX_CLIENTEDGE,"Leszek","Okno 3",WS_VISIBLE|WS_OVERLAPPEDWINDOW,  // |WS_CHILD |WS_POPUP
					200, /* x */
					200, /* y */
					800, /* width */
					600, /* height */
					NULL,NULL,hIns,NULL);
				
				break;	
			}
			
		}
		break;	
	}

	case WM_CREATE: {
		BudujGuziki(h1);
		break;
	}
		/* All other messages (a lot of them) are processed using default procedures */
		default:
			return DefWindowProc(h1, Message, wParam, lParam);
			
			
	}
	return 0;
}

LRESULT CALLBACK WndProc2(HWND h2, UINT Message, WPARAM wParam, LPARAM lParam) { //2
    PAINTSTRUCT ps; 
    RECT clientRect;
    HDC hdc; 
    HPEN hPen;
    int x;
    int y;
    float radByPxX;
    float radByPxY;
    float rad;

	switch(Message) {
		
		/* Upon destruction, tell the main thread to stop */
		case WM_DESTROY: {
			//PostQuitMessage(0);
			break;
		}
		
		case WM_CHAR: {
			if (MessageBox(h2,"Czy chcesz zamknac okno?","Zamkniecie",MB_OKCANCEL|MB_ICONASTERISK|MB_DEFBUTTON2) == IDOK)			
				//PostQuitMessage(0);
			break;
		} 
		
		case WM_PAINT: {
			GetClientRect(h2,&clientRect);
			hdc = BeginPaint(h2, &ps); 
            hPen = CreatePen(PS_SOLID,5,RGB(0,0,255));
            SelectObject(hdc, hPen);
            x = clientRect.right - clientRect.left;
            y = clientRect.bottom - clientRect.top;
			rad = 3.14 / 180;
			
			MoveToEx(hdc, 0, 0.5*y, NULL);
			LineTo(hdc,x,0.5*y);
			MoveToEx(hdc,0.5*x,0, NULL);
			LineTo(hdc,0.5*x,y);
			
			MoveToEx(hdc,0.98*x,0.48*y,NULL);
			LineTo(hdc,x,0.5*y);
			MoveToEx(hdc,0.98*x,0.52*y,NULL);
			LineTo(hdc,x,0.5*y);
			
			MoveToEx(hdc,0.48*x,0.02*y,NULL);
			LineTo(hdc,0.5*x,0);
			MoveToEx(hdc,0.52*x,0.02*y,NULL);
			LineTo(hdc,0.5*x,0); 
			
			double radian = 3.1415/180;
			MoveToEx(hdc,clientRect.left,clientRect.bottom/2,NULL);
			for (int i = 0;i<360;i++)
			{
				//i=i-clientRect.left/2;
				float y = sin(i* radian)*180;
				LineTo(hdc,i+clientRect.right/2,clientRect.bottom/2-y);
				MoveToEx(hdc,i+clientRect.right/2,clientRect.bottom/2-y,NULL);
			}
			
            EndPaint(h2, &ps); 		
			break;
		}
		
		/* All other messages (a lot of them) are processed using default procedures */
		default:
			return DefWindowProc(h2, Message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WndProc3(HWND h3, UINT Message, WPARAM wParam, LPARAM lParam) { //3
	PAINTSTRUCT ps; 
    RECT clientRect;
    HDC hdc,hdc_lines; 
    HPEN hPen, hPen_lines,hPen_wykres;
    int x;
    int y;
    float radByPxX;
    float radByPxY;
    float rad;
   // tempList.clear();
	DatabaseReaderTemp();
	
	switch(Message) {
		
		case WM_PAINT: {
			GetClientRect(h3,&clientRect);
			hdc = BeginPaint(h3, &ps); 
			hdc_lines = BeginPaint(h3, &ps); 
            hPen = CreatePen(PS_SOLID,4,RGB(0,0,255));
            hPen_lines = CreatePen(PS_DASH,1,RGB(0,0,0));
            hPen_wykres = CreatePen(PS_SOLID,4,RGB(0,0,0));
            
            SelectObject(hdc, hPen_wykres);
            x = clientRect.right - clientRect.left;
            y = clientRect.bottom - clientRect.top;
			rad = 3.14 / 180;
			
			MoveToEx(hdc, 0, 0.5*y, NULL);
			LineTo(hdc,x,0.5*y);
			MoveToEx(hdc,0.1*x,0, NULL);
			LineTo(hdc,0.1*x,y);
			
			MoveToEx(hdc,0.98*x,0.48*y,NULL);
			LineTo(hdc,x,0.5*y);
			MoveToEx(hdc,0.98*x,0.52*y,NULL);
			LineTo(hdc,x,0.5*y);
			
			MoveToEx(hdc,0.08*x,0.02*y,NULL);
			LineTo(hdc,0.1*x,0);
			MoveToEx(hdc,0.12*x,0.02*y,NULL);
			LineTo(hdc,0.1*x,0); 
			
			//MoveToEx(hdc,clientRect.left+75,clientRect.bottom/2,NULL);
			int scale_y = (clientRect.bottom/2) / 40 ; // skala dla 40 stopni
			int scale_x = (clientRect.right) / 10  ; // dla 60 pomiarów
			int mid_y = (clientRect.bottom/2);
			SelectObject(hdc, hPen_lines);
			
			int i = 1;
			for( std::list<std::string>::iterator iter=tempList.begin(); iter != tempList.end(); iter++ )
			{
				if(i>1)
				{
					MoveToEx(hdc,i*scale_x,0,NULL);
					LineTo  (hdc,i*scale_x,clientRect.bottom);
				}
				i++;
			}
			int linie_poziome = clientRect.bottom+200/(clientRect.bottom/80);
			for( int j = linie_poziome ; j > 0 ; j-=10 )
			{
				
				MoveToEx(hdc,0.1*x,j*scale_y-88,NULL);
				LineTo  (hdc,clientRect.right,j*scale_y-88);
				
			}
			
			//MessageBox(h1, (char*)tempList.size(),"Temperatura w czestochowie!",MB_OK);
		
			//tempList.reverse();
		
			SelectObject(hdc, hPen);
			i=1;//tempList.size();
			for( std::list<std::string>::reverse_iterator iter=tempList.rbegin(); iter !=tempList.rend() ; iter++ )
			{
				if(i == 1)
				{
					//iter--;
					MoveToEx(hdc,i*scale_x,mid_y+(-scale_y * atoi ((*iter).c_str())),NULL);
					
				}
				//MessageBox(h1, "fd","Temperatura w czestochowie!",MB_OK);	
				LineTo  (hdc,i*scale_x,mid_y+(-scale_y * atoi ((*iter).c_str())));
				MoveToEx(hdc,i*scale_x,mid_y+(-scale_y * atoi ((*iter).c_str())),NULL);
				
				//pionowe linie
				if(i==10)
					break;
				//tempList.pop_back();
				//MessageBox(h1, (*iter).c_str(),"Temperatura w czestochowie!",MB_OK);
				i++;
			}
      		
			//double radian = 3.1415/180;
			//MoveToEx(hdc,clientRect.left,clientRect.bottom/2,NULL);
			//for (int i = 0;i<360;i++)
			//{
			//	//i=i-clientRect.left/2;
			//	float y = sin(i* radian)*180;
			//	LineTo(hdc,i+clientRect.right/2,clientRect.bottom/2-y);
			//	MoveToEx(hdc,i+clientRect.right/2,clientRect.bottom/2-y,NULL);
			//}
			
            EndPaint(h3, &ps); 		
			break;
		}
		
		/* All other messages (a lot of them) are processed using default procedures */
		default:
			return DefWindowProc(h3, Message, wParam, lParam);
	}
	return 0;
}

/* The 'main' function of Win32 GUI programs: this is where execution starts */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	
	    // Initialize the critical section one time only.
    if (!InitializeCriticalSectionAndSpinCount(&CriticalSection, 
        0x00000400) ) 
        return -1;
	
	WNDCLASSEX wc1; /* A properties struct of our window */
	WNDCLASSEX wc2;
	
	HWND hwnd; /* A 'HANDLE', hence the H, or a pointer to our window */
	MSG msg; /* A temporary location for all messages */

	hIns = hInstance;

	/* zero out the struct and set the stuff we want to modify */
	memset(&wc1,0,sizeof(wc1));
	wc1.cbSize		 = sizeof(WNDCLASSEX);
	wc1.lpfnWndProc	 = WndProc1; /* This is where we will send messages to */
	wc1.hInstance	 = hInstance;
	wc1.hCursor		 = LoadCursor(NULL, IDC_ARROW);

	memset(&wc2,0,sizeof(wc2));
	wc2.cbSize		 = sizeof(WNDCLASSEX);
	wc2.lpfnWndProc	 = WndProc2; /* This is where we will send messages to */
	wc2.hInstance	 = hInstance;
	wc2.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	
	
	
	/* White, COLOR_WINDOW is just a #define for a system color, try Ctrl+Clicking it */
	wc1.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc1.lpszClassName = "Mietek";
	wc1.hIcon		 = LoadIcon(NULL, IDI_APPLICATION); /* Load a standard icon */
	wc1.hIconSm		 = LoadIcon(NULL, IDI_APPLICATION); /* use the name "A" to use the project icon */

	wc2.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc2.lpszClassName = "Zbyszek";
	wc2.hIcon		 = LoadIcon(NULL, IDI_APPLICATION); /* Load a standard icon */
	wc2.hIconSm		 = LoadIcon(NULL, IDI_APPLICATION); /* use the name "A" to use the project icon */
	WNDCLASSEX wc3;
			
	memset(&wc3,0,sizeof(wc3));
	wc3.cbSize		 = sizeof(WNDCLASSEX);
	wc3.lpfnWndProc	 = WndProc3; /* This is where we will send messages to */
	wc3.hInstance	 = hInstance;
	wc3.hCursor		 = LoadCursor(NULL, IDC_ARROW);	
	wc3.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc3.lpszClassName = "Leszek";
	wc3.hIcon		 = LoadIcon(NULL, IDI_APPLICATION); /* Load a standard icon */
	wc3.hIconSm		 = LoadIcon(NULL, IDI_APPLICATION); /* use the name "A" to use the project icon */	


	if(!RegisterClassEx(&wc1)) {
		MessageBox(NULL, "Window Registration Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}

	if(!RegisterClassEx(&wc2)) {
		MessageBox(NULL, "Window Registration Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}
	
	if(!RegisterClassEx(&wc3)) {
		MessageBox(NULL, "Window Registration Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}		

	h1 = CreateWindowEx(WS_EX_CLIENTEDGE,"Mietek","Okno 1",WS_VISIBLE|WS_OVERLAPPEDWINDOW,
		0, /* x */
		0, /* y */
		640, /* width */
		480, /* height */
		NULL,NULL,hInstance,NULL);

	h2 = CreateWindowEx(WS_EX_CLIENTEDGE,"Zbyszek","Okno 2",WS_OVERLAPPEDWINDOW,
		100, /* x */
		100, /* y */
		800, /* width */
		600, /* height */
		NULL,NULL,hInstance,NULL);



	SetTimer(h1, IDT_TIMER1, 500, (TIMERPROC) NULL);

	if(hwnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}

	/*
		This is the heart of our program where all input is processed and 
		sent to WndProc. Note that GetMessage blocks code flow until it receives something, so
		this loop will not produce unreasonably high CPU usage
	*/
	while(GetMessage(&msg, NULL, 0, 0) > 0) { /* If no error is received... */
		TranslateMessage(&msg); /* Translate key codes to chars if present */
		DispatchMessage(&msg); /* Send it to WndProc */
	}
	
	   // Release resources used by the critical section object.
    DeleteCriticalSection(&CriticalSection);
	
	return msg.wParam;
}
