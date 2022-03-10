This project runs on Linux platform.

1.The directory "module" contains the modules generated from the source code.
  (1)ms-lite: Message Server(MS)
  (2)mc-lite: Message Client(MC)

2.The directory "demo_picture" contains the demo execution result.
  This demo contains three members:alice, bob and tom.

3.MC operation command.
  (1)Excute the client:
       ./mc-lite --name alice
  (2)Connect to the server:
       HI 127.0.0.1:12345
  (3)Send message to bob:
       @bob hi bob
  (4)Broadcast:
       @all hi all
  (5)disconnect to the server:
       BYE
  (6)Quit the client:
       QUIT