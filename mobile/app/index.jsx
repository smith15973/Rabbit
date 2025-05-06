import { useState } from "react";
import { Button, Text, View } from "react-native";
import { GestureHandlerRootView } from "react-native-gesture-handler";
import { ReactNativeJoystick } from "@korsolutions/react-native-joystick";
import { runOnJS } from "react-native-reanimated";
import { LogBox } from "react-native";

// Add this at the top of your file to ignore the specific warning
LogBox.ignoreLogs([
  "[react-native-gesture-handler] None of the callbacks in the gesture are worklets"
]);

export default function Index() {
  const [on, setOn] = useState(false);
  const [joystickData, setJoystickData] = useState({ angle: 0, distance: 0 });

  const handleToggleStartPress = () => {
    setOn(!on);
    console.log(!on);
  };

  // Create a JS thread function to handle joystick data
  const handleJoystickMove = (data) => {
    runOnJS(true);
    console.log(data);
  };

  return (
    <GestureHandlerRootView style={{ flex: 1 }}>
      <View
        style={{
          flex: 1,
          justifyContent: "center",
          alignItems: "center",
        }}
      >
        <Text>Edit app/index.tsx to edit this screen. WOW</Text>
        <Button title={on ? "Off" : "On"} onPress={handleToggleStartPress}></Button>
        
        <ReactNativeJoystick 
          color="#06b6d4" 
          radius={75} 
          // Use runOnJS to explicitly run the callback on JS thread
          onMove={(data) => {
            'worklet';
            runOnJS(true);
            runOnJS(handleJoystickMove)(data);
          }} 
        />
        
        <Text style={{ marginTop: 20 }}>
          {/* Angle: {joystickData.angle.toFixed(2)}° */}
        </Text>
      </View>
    </GestureHandlerRootView>
  );
}