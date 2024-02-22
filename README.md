# toy_car 
platform: Raspberry Pi 5

# dependency 1: lgpio libaray
*manipulate GPIO on raspberry pi 5*

```
wget https://github.com/joan2937/lg/archive/master.zip
unzip master.zip
cd lg-master
make
sudo make install
```
# how to build and run

## build
```
bash build-test.sh
```

## run
1. run in front end
   ```
   ./test
   ```
3. run in back end
   ```
   nohup ./test 2>&1 &
   ```

# supported features
1. Use AT8236 to drive 4wd toy car. Support move forward, move backward, turn left, turn right, brake
2. Support multiple command input. Such as linux terminal, bluetooth joystick, infrared detector
