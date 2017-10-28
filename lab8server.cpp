#ifdef WIN32
#define _WIN32_WINNT 0x0501
#include <stdio.h>
#endif


#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <vector>
#include <string>
using namespace boost::asio;
using namespace boost::posix_time;
std::vector<std::string> clients;
io_service service;
static unsigned int index = { 0 }; // Индекс/id подключенного клиента

#define MEM_FN(x)       boost::bind(&self_type::x, shared_from_this())
#define MEM_FN1(x,y)    boost::bind(&self_type::x, shared_from_this(),y)
#define MEM_FN2(x,y,z)  boost::bind(&self_type::x, shared_from_this(),y,z)


class talk_to_client : public boost::enable_shared_from_this<talk_to_client>, boost::noncopyable
{
	typedef talk_to_client self_type;
	talk_to_client() : sock_(service), started_(false) {}
public:
	typedef boost::system::error_code error_code;
	typedef boost::shared_ptr<talk_to_client> ptr;

	void start()
	{
		started_ = true;
		do_read();
	}
	static ptr new_()
	{
		ptr new_(new talk_to_client);
		return new_;
	}
	void stop()
	{
		if (!started_) return;
		started_ = false;
		sock_.close();
	}
	ip::tcp::socket & sock() { return sock_; }
private:
	void on_read(const error_code & err, size_t bytes)
	{
		if (!err)
		{
			SetConsoleOutputCP(65001);
			index++;
			std::cout << std::endl << "Работа с клиентом id = " << index << std::endl;
			std::string msg(read_buffer_, bytes);
			std::string all_clients;
			if (msg.size() > 1023)
			{
				std::cout << "[-] Слишком длинные инициалы! Соединение прервано." << std::endl;
				stop();
				return;
			}
			if (msg == "admin admin'") // Проверка на подключение админа
			{
				clients.push_back(msg); // Добавляем к клиентам админа
				if (all_clients.size() > 1023)
					all_clients.clear(); // Очистка истории о клиентах, когда буфер не может больше содержать больше информации
				for (unsigned int i = { 0 }; i < clients.size(); ++i)
				{
					all_clients += clients[i]; // Перечисление всех клиентов
				}
				SetConsoleOutputCP(1251);
				//std::cout << all_clients; - для просмотра того, как сообщение представляется на сервере
				do_write(all_clients); // Передаем админу инф-ю о клиентах
				clients[clients.size() - 1][msg.size() - 1] = '\n'; // Убираем ' с конца списка клиентов, так как нам понадобится дополнять этот список 
																	// и выдавать его при новом подключении админа, иначе список выведится не весь, 
																	// а только его первая часть
			}
			else
			{
				clients.push_back(msg); // Заполняем инф-ю о клиентах
				clients[clients.size() - 1][msg.size() - 1] = '\n'; // Ставим вместо ' на конце сообщения от каждого клиента \n 
				do_write(msg + "\n"); // Передаем сообщение клиенту
			}
		}
		stop();
	}

	void on_write(const error_code & err, size_t bytes)
	{
		do_read();
	}
	void do_read()
	{
		async_read(sock_, buffer(read_buffer_), MEM_FN2(read_complete, _1, _2), MEM_FN2(on_read, _1, _2));
	}
	void do_write(const std::string & msg)
	{
		std::copy(msg.begin(), msg.end(), write_buffer_);
		sock_.async_write_some(buffer(write_buffer_, msg.size()), MEM_FN2(on_write, _1, _2));
	}
	size_t read_complete(const boost::system::error_code & err, size_t bytes)
	{
		if (err) return 0;
		bool found = std::find(read_buffer_, read_buffer_ + bytes, '\'') < read_buffer_ + bytes;
		// Последовательное небуфферизированное чтение до \n
		return found ? 0 : 1;
	}
private:
	ip::tcp::socket sock_;
	enum { max_msg = 1024 };
	char read_buffer_[max_msg];
	char write_buffer_[max_msg];
	bool started_;
};

ip::tcp::acceptor acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), 8001));
// Серверные TCP функции в asio выполняет объект класса boost::asio::ip::tcp::acceptor
// Собственно этот объект открывает соединение

void handle_accept(talk_to_client::ptr client, const boost::system::error_code & err)
{
	client->start();
	talk_to_client::ptr new_client = talk_to_client::new_();
	acceptor.async_accept(new_client->sock(), boost::bind(handle_accept, new_client, _1));
}


int main()
{
	SetConsoleOutputCP(65001);
	std::cout << std::endl << "*** Лабораторная работа №8: Реализация сервера Connection logger ***" << std::endl;
	talk_to_client::ptr client = talk_to_client::new_();
	acceptor.async_accept(client->sock(), boost::bind(handle_accept, client, _1));
	// Метод accept объекта класса acceptor с объектом класса socket переданным в качестве параметра, 
	// начинает ожидание подключения, что при синхронной схеме работы приводит к блокированию потока до тех пор, 
	// пока какой-нибудь клиент не осуществит подключения
	service.run();

	return 0;
}

