CXX ?= g++

server : main.cpp ./Config/Config.cpp ./http_conn/http_conn.cpp ./log/log.cpp ./sqlconnpool/sqlconnpool.cpp ./timer/heaptimer.cpp ./webserver/webserver.cpp ./Config/Json.cpp ./Config/JsonValue.cpp ./Config/JsonParse.cpp
	$(CXX) -o server $^ -lpthread -lmysqlclient

clean:
	rm -r server
