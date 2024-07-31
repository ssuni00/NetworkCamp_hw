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
   <img width="1440" alt="image" src="https://github.com/user-attachments/assets/b058d7ac-ddcf-4710-b416-e87f7c29681e">
   

## NetworkCamp_Day3
Simple Remote Shell 구현

* 사용법
- cd <dir name>
- dl/up <file name>
- ls, ls -al, ls -l, ...
- <img width="547" alt="image" src="https://github.com/user-attachments/assets/ccd82558-52c0-400f-ad08-6f56ac74530f">
- <img width="465" alt="image" src="https://github.com/user-attachments/assets/447b2506-c0fc-4834-8864-4bb96556a457">
- <img width="374" alt="image" src="https://github.com/user-attachments/assets/79d060f3-77c3-448b-b927-6368d0dc6999">






