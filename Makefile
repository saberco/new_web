CXX ?= g++

server : main.cpp ./Config/Config.cpp ./http_conn/http_conn.cpp ./log/log.cpp ./sqlconnpool/sqlconnpool.cpp ./timer/timerlst.cpp ./webserver/webserver.cpp
	$(CXX) -o server $^ -lpthread -lmysqlclient

clean:
	rm -r server
