# talker and listener 
Steps to run talker and listener scripts :
* To run the talker and listener first we need to copy the 1_chatter_listener to ros2_ws src

```
cp -r /home/(user_name)/MARIO/1_chatter_listener /home/(user_name)/ros2_ws/src
```
* To build the necesarry packages we need to install them so for that go to ros2_ws
```
cd ros2_ws
```
* Then to build run:
```
colcon build
```
* Now we need to source it
```
source install/setup.bash
```
* Now run the talker
```
ros2 run chatter_listener talker.py
```
* To run the listener `open another terminal` and again source it:
```
source install/setup.bash
```
* Now run the listener
```
ros2 run chatter_listener listener.py
```
* TALKER AND LISTENER
<p align="centere"><img src ="assets/talker_listener.gif" width="1500" height="900"></p>

