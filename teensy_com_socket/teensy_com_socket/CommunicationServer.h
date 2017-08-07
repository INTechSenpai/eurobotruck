#ifndef _COMMUNICATIONSERVER_h
#define _COMMUNICATIONSERVER_h

#include <Ethernet.h>
#include "Command.h"
#include <Printable.h>
#include <stdio.h>
#include <stdarg.h>
#include <vector>


/* Configuration r�seau */
#define MAC_ADDR	0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
#define IP_ADDR		172, 16, 0, 2
#define DNS_IP		172, 16, 0, 1
#define GATEWAY_IP	172, 16, 0, 1
#define SUBNET_MASK	255, 255, 255, 0
#define TCP_PORT	80

/* Activation de la liaison s�rie de secours */
#define SERIAL_ENABLE 1

/* Pin mapping */
//Todo: � d�placer dans le fichier appropri�
#define WIZ820_SS_PIN		10
#define WIZ820_MOSI_PIN		11
#define WIZ820_MISO_PIN		12
#define WIZ820_SCLK_PIN		13
#define WIZ820_RESET_PIN	14
#define WIZ820_PWDN_PIN		15

/* Configurations diverses */
#define COMMAND_BUFFER_SIZE		16
#define OUTPUT_BUFFER_SIZE		255
#define HEADER_BYTE				0xFF
#define DEFAULT_SUSCRIPTION		0x06
#define MAX_RECEPTION_DURATION	500		// �s
#define ASYNC_TRACE_FILENAME	"ISR"
#define CHANNEL_MAX_NB			32


enum Channel
{
	ODOMETRY_AND_SENSORS	= 0x00,
	INFO					= 0x01,
	ERROR					= 0x02,
	TRACE					= 0x03,
	DIRECTION				= 0x04,
	AIM_TRAJECTORY			= 0x05,
	PID_SPEED				= 0x06,
	PID_TRANS				= 0x07,
	PID_TRAJECTORY			= 0x08,
	BLOCKING_MGR			= 0x09,
	STOPPING_MGR			= 0x0A
};


class CommunicationServer
{
public:
	/* Constructeur */
	CommunicationServer();

	/* Envoie les messages de la file d'attente et lit les messages entrants */
	void communicate();

	/* Renvoie le nombre d'ordres pr�sents dans le buffer de r�ception */
	uint8_t available();

	/* Indique si le client donn� est connect� ou non au serveur */
	bool isConnected(uint8_t client);

	/* 
		Revoie la commande la plus ancienne du buffer de r�ception, et la retire du buffer
		Si le buffer est vide, la commande retourn�e aura l'attribut "non valide"
	*/
	Command getLastCommand();

	/* Envoie la commande pass�e en argument, avec une trame standard */
	void sendAnswer(Command answer);

	/* Envoi de donn�es spontan�es avec une trame standard */
	void sendData(Channel channel, std::vector<uint8_t> const & data);

	/* M�thodes permettant l'envoi de donn�es spontan�es avec des trames d'information */
	void print(uint32_t n) { print(INFO, n); }
	void print(int32_t n) { print(INFO, n); }
	void print(double d) { print(INFO, d); }
	void println(uint32_t n) { print(INFO, n, true); }
	void println(int32_t n) { print(INFO, n, true); }
	void println(double d) { print(INFO, d, true); }
	void println() { println(INFO); }

	void print_err(uint32_t n) { print(ERROR, n); }
	void print_err(int32_t n) { print(ERROR, n); }
	void print_err(double d) { print(ERROR, d); }
	void println_err(uint32_t n) { print(ERROR, n, true); }
	void println_err(int32_t n) { print(ERROR, n, true); }
	void println_err(double d) { print(ERROR, d, true); }
	void println_err() { println(ERROR); }

	void print(const Printable & obj) { print(INFO, obj); }
	void println(const Printable & obj) { print(obj); println(); }
	void println(Channel channel, const Printable & obj) { print(channel, obj); println(channel); }

	void print(Channel channel, const Printable & obj);
	void printf(const char* format, ...);
	void printf_err(const char* format, ...);
	void printf(Channel channel, const char* format, ...);
private:
	void print(Channel channel, uint32_t u, bool newLine = false);
	void print(Channel channel, int32_t n, bool newLine = false);
	void print(Channel channel, double d, bool newLine = false);
	void print(Channel channel, const char* str, bool newLine = false);
	void println(Channel channel);

	/* Envoie les 3 octets d'ent�te d'une trame d'information */
	void printHeader(Stream & stream, uint8_t id)
	{
		stream.write(0xFF);
		stream.write(id);
		stream.write(0xFF);
	}

public:
	/* Envoie une trame d'information sur la canal TRACE permettant de retrouver la ligne de code et le fichier ayant appel� la m�thode */
	void trace(uint32_t line, const char* filename, uint32_t timestamp = 0);

	/* M�me utilisation que trace() mais utilisable depuis une interruption (le message sera envoy� plus tard, depuis la boucle principale) */
	void asynchronous_trace(uint32_t line);

private:
	/* Envoie la chaine de caract�res contenue dans outputBuffer sous forme de trame d'information */
	void sendOutputBuffer(Channel channel);

	/* Envoi d'un octet � un destinataire */
	size_t sendByte(uint8_t byte, uint8_t dest);

	/* Envoi d'un vecteur d'octets � un destinataire */
	size_t sendVector(std::vector<uint8_t> const & vect, uint8_t dest);

	/* Envoi d'une chaine de caract�res C � un destinataire (avec le caract�re de fin de chaine) */
	size_t sendCString(const char* str, uint8_t dest);

	/*
		Si la commande concerne une inscription/d�sinscription, mets � jour la subscriptionList
		Sinon, ajoute la commande au buffer des "commandes en attente d'ex�cution" 
	*/
	void processOrAddCommandToBuffer(Command command);

	/* Indique si le client est abonn� � cette chaine */
	bool subscribed(uint8_t client, Channel channel)
	{
		return subscriptionList[client] & ((uint32_t)1 << (uint8_t)channel);
	}

	/* Indique si l'un des clients est abonn� � cette chaine */
	bool isThereListener(Channel channel)
	{
		for (uint8_t i = 0; i < MAX_SOCK_NUM + 1; i++)
		{
			if (subscribed(i, channel))
			{
				return true;
			}
		}
		return false;
	}


	class CommandReceptionHandler
	{
	public:
		CommandReceptionHandler()
		{
			commandAvailable = false;
			receptionStarted = false;
		}

		/* Renvoie 0 si l'octet a �t� ajout�, 1 sinon. Revoie -1 en cas de r�ception d'une trame d'information */
		int8_t addByte(uint8_t newByte, uint8_t source)
		{
			if (!commandAvailable)
			{
				if (receptionStarted)
				{
					receptionBuffer.push_back(newByte);
					if (receptionBuffer.size() > 1 && receptionBuffer.at(1) > COMMAND_MAX_DATA_SIZE)
					{
						receptionBuffer.clear();
						receptionStarted = false;
						return -1;
					}
					else if (receptionBuffer.size() >= 2 && receptionBuffer.at(1) == receptionBuffer.size() - 2)
					{
						lastCommand = Command(source, receptionBuffer);
						receptionBuffer.clear();
						commandAvailable = true;
						receptionStarted = false;
					}
				}
				else if(newByte == HEADER_BYTE)
				{
					receptionStarted = true;
				}
				else
				{
					return 1;
				}
			}
			return 0;
		}

		bool available()
		{
			return commandAvailable;
		}
		
		Command getCommand()
		{
			commandAvailable = false;
			return lastCommand;
		}

	private:
		std::vector<uint8_t> receptionBuffer;
		bool commandAvailable;
		bool receptionStarted;
		Command lastCommand;
	};

	struct ExecTrace
	{
		uint32_t lineNb;
		uint32_t timestamp;
	};

	char outputBuffer[OUTPUT_BUFFER_SIZE];
	EthernetServer ethernetServer;
	Command commandBuffer[COMMAND_BUFFER_SIZE];
	uint8_t cBufferHead;
	uint8_t cBufferTail;

	CommandReceptionHandler receptionHandlers[MAX_SOCK_NUM + 1];
	EthernetClient ethernetClients[MAX_SOCK_NUM];
	uint32_t subscriptionList[MAX_SOCK_NUM + 1];

	std::vector<ExecTrace> asyncTraceVect;
	std::vector<ExecTrace> asyncTraceVectBis;
	volatile bool bisTraceVectUsed;
};


extern CommunicationServer Server;


#endif
