# NetworkCamp_hw

## NetworkCamp_Day1

* 1. 클라이언트가 서버에 접속 (TCP 이용)
* 2. 서버프로그램이실행중인디렉토리의모든파일목록(파일이름, 파일 크기)을 클라이언트에게 전송
* 3. 클라이언트는서버가보내온목록을보고파일하나를선택
* 4. 서버는 클라이언트가 선택한 파일을 클라이언트에게 전송
* 5. 전송된 파일은 클라이언트 프로그램이 실행 중인 디렉토리에 동일한 이름으로 저장됨.
* 2~5 반복

  <img width="296" alt="image" src="https://github.com/user-attachments/assets/b9b1e544-97bd-41f3-a00f-c7905ce8fa4d">
  
## NetworkCamp_Day2

Stop-and-Wait Protocol 구현

* UDP통신을 사용하는 uecho_server.c와 uecho_client.c는 신뢰성있는 데이터전송을 보장하지 않기에 (손실된 패킷을 복구하는 기능이 없음)
* 이를보완하여 신뢰성있는 데이터전송을 보장하는 Stop-and-WaitProtocol 기반의 프로그램 구현
* 파일전송이 완료되면 Throughput 출력 (Throughput = 받은데이터양 / 전송시간)


1 시도
<img width="1139" alt="image" src="https://github.com/user-attachments/assets/cd119b03-07ed-488b-8f83-b40ede836c95">
: 1. 일단 파일말고 메세지로 client -> server (clnt: serv야 나 접속했어! 암레디!)
  2. 메세지로 sequence, ack 잘 오고가는지 확인

2시도 (파일을 보내보자)
<img width="1440" alt="image" src="https://github.com/user-attachments/assets/b058d7ac-ddcf-4710-b416-e87f7c29681e">
: 일단 파일은 잘 가는데.. 52KB파일을 512byte 기준으로 잘라서 전송
로스가 걸려있는데 제대로 간거면 맞게간..?


