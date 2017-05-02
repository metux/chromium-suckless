// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/emulator/device_emulator_message_handler.h"

#include <stdint.h>
#include <utility>

#include "ash/common/system/tray/system_tray_delegate.h"
#include "ash/common/wm_shell.h"
#include "base/bind.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/chromeos/system/fake_input_device_settings.h"
#include "chrome/browser/chromeos/system/input_device_settings.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_cras_audio_client.h"
#include "content/public/browser/web_ui.h"
#include "device/bluetooth/dbus/bluez_dbus_manager.h"
#include "device/bluetooth/dbus/fake_bluetooth_adapter_client.h"
#include "device/bluetooth/dbus/fake_bluetooth_device_client.h"

namespace {

// Define the name of the callback functions that will be used by JavaScript.
const char kInitialize[] = "initializeDeviceEmulator";
const char kBluetoothDiscoverFunction[] = "requestBluetoothDiscover";
const char kBluetoothPairFunction[] = "requestBluetoothPair";
const char kRequestBluetoothInfo[] = "requestBluetoothInfo";
const char kRequestPowerInfo[] = "requestPowerInfo";
const char kRequestAudioNodes[] = "requestAudioNodes";

// Define update function that will update the state of the audio ui.
const char kInsertAudioNode[] = "insertAudioNode";
const char kRemoveAudioNode[] = "removeAudioNode";

// Define update functions that will update the power properties to the
// variables defined in the web UI.
const char kRemoveBluetoothDevice[] = "removeBluetoothDevice";
const char kUpdateTimeToEmpty[] = "updateTimeToEmpty";
const char kUpdateTimeToFull[] = "updateTimeToFull";
const char kUpdatePowerSources[] = "updatePowerSources";
const char kUpdatePowerSourceId[] = "updatePowerSourceId";
const char kSetHasTouchpad[] = "setHasTouchpad";
const char kSetHasMouse[] = "setHasMouse";

// Define callback functions that will update the JavaScript variable
// and the web UI.
const char kUpdateAudioNodes[] =
    "device_emulator.audioSettings.updateAudioNodes";
const char kAddBluetoothDeviceJSCallback[] =
    "device_emulator.bluetoothSettings.addBluetoothDevice";
const char kDevicePairedFromTrayJSCallback[] =
    "device_emulator.bluetoothSettings.devicePairedFromTray";
const char kDeviceRemovedFromMainAdapterJSCallback[] =
    "device_emulator.bluetoothSettings.deviceRemovedFromMainAdapter";
const char kPairFailedJSCallback[] =
    "device_emulator.bluetoothSettings.pairFailed";
const char kUpdateBluetoothInfoJSCallback[] =
    "device_emulator.bluetoothSettings.updateBluetoothInfo";
const char kTouchpadExistsCallback[] =
    "device_emulator.inputDeviceSettings.setTouchpadExists";
const char kMouseExistsCallback[] =
    "device_emulator.inputDeviceSettings.setMouseExists";

const char kPairedPropertyName[] = "Paired";

// Wattages to use as max power for power sources.
const double kPowerLevelHigh = 50;
const double kPowerLevelLow = 2;

}  // namespace

namespace chromeos {

class DeviceEmulatorMessageHandler::BluetoothObserver
    : public bluez::BluetoothDeviceClient::Observer {
 public:
  explicit BluetoothObserver(DeviceEmulatorMessageHandler* owner)
      : owner_(owner) {
    owner_->fake_bluetooth_device_client_->AddObserver(this);
  }

  ~BluetoothObserver() override {
    owner_->fake_bluetooth_device_client_->RemoveObserver(this);
  }

  // chromeos::BluetoothDeviceClient::Observer.
  void DeviceAdded(const dbus::ObjectPath& object_path) override;

  // chromeos::BluetoothDeviceClient::Observer.
  void DevicePropertyChanged(const dbus::ObjectPath& object_path,
                             const std::string& property_name) override;

  // chromeos::BluetoothDeviceClient::Observer.
  void DeviceRemoved(const dbus::ObjectPath& object_path) override;

 private:
  DeviceEmulatorMessageHandler* owner_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothObserver);
};

void DeviceEmulatorMessageHandler::BluetoothObserver::DeviceAdded(
    const dbus::ObjectPath& object_path) {
  std::unique_ptr<base::DictionaryValue> device =
      owner_->GetDeviceInfo(object_path);

  // Request to add the device to the view's list of devices.
  owner_->web_ui()->CallJavascriptFunctionUnsafe(kAddBluetoothDeviceJSCallback,
                                                 *device);
}

void DeviceEmulatorMessageHandler::BluetoothObserver::DevicePropertyChanged(
  const dbus::ObjectPath& object_path,
  const std::string& property_name) {
  if (property_name == kPairedPropertyName) {
    owner_->web_ui()->CallJavascriptFunctionUnsafe(
        kDevicePairedFromTrayJSCallback,
        base::StringValue(object_path.value()));
  }
}

void DeviceEmulatorMessageHandler::BluetoothObserver::DeviceRemoved(
    const dbus::ObjectPath& object_path) {
  owner_->web_ui()->CallJavascriptFunctionUnsafe(
      kDeviceRemovedFromMainAdapterJSCallback,
      base::StringValue(object_path.value()));
}

class DeviceEmulatorMessageHandler::CrasAudioObserver
    : public CrasAudioClient::Observer {
 public:
  explicit CrasAudioObserver(DeviceEmulatorMessageHandler* owner)
      : owner_(owner) {
    owner_->fake_cras_audio_client_->AddObserver(this);
  }

  ~CrasAudioObserver() override {
    owner_->fake_cras_audio_client_->RemoveObserver(this);
  }

  // chromeos::CrasAudioClient::Observer.
  void NodesChanged() override;

 private:
  DeviceEmulatorMessageHandler* owner_;
  DISALLOW_COPY_AND_ASSIGN(CrasAudioObserver);
};

void DeviceEmulatorMessageHandler::CrasAudioObserver::NodesChanged() {
  owner_->HandleRequestAudioNodes(nullptr);
}

DeviceEmulatorMessageHandler::DeviceEmulatorMessageHandler()
    : fake_bluetooth_device_client_(
          static_cast<bluez::FakeBluetoothDeviceClient*>(
              bluez::BluezDBusManager::Get()
                  ->GetBluetoothDeviceClient())),
      fake_cras_audio_client_(static_cast<chromeos::FakeCrasAudioClient*>(
          chromeos::DBusThreadManager::Get()
              ->GetCrasAudioClient())),
      weak_ptr_factory_(this) {}

DeviceEmulatorMessageHandler::~DeviceEmulatorMessageHandler() {
}

void DeviceEmulatorMessageHandler::Init(const base::ListValue* args) {
  AllowJavascript();
}

void DeviceEmulatorMessageHandler::RequestPowerInfo(
    const base::ListValue* args) {
}

void DeviceEmulatorMessageHandler::HandleRemoveBluetoothDevice(
    const base::ListValue* args) {
  std::string path;
  CHECK(args->GetString(0, &path));
  fake_bluetooth_device_client_->RemoveDevice(
      dbus::ObjectPath(bluez::FakeBluetoothAdapterClient::kAdapterPath),
      dbus::ObjectPath(path));
}

void DeviceEmulatorMessageHandler::HandleRequestBluetoothDiscover(
    const base::ListValue* args) {
  CreateBluetoothDeviceFromListValue(args);
}

void DeviceEmulatorMessageHandler::HandleRequestBluetoothInfo(
    const base::ListValue* args) {
  // Get a list containing paths of the devices which are connected to
  // the main adapter.
  std::vector<dbus::ObjectPath> paths =
      fake_bluetooth_device_client_->GetDevicesForAdapter(
          dbus::ObjectPath(bluez::FakeBluetoothAdapterClient::kAdapterPath));

  base::ListValue devices;
  // Get each device's properties.
  for (const dbus::ObjectPath& path : paths) {
    std::unique_ptr<base::DictionaryValue> device = GetDeviceInfo(path);
    devices.Append(std::move(device));
  }

  std::unique_ptr<base::ListValue> predefined_devices =
      fake_bluetooth_device_client_->GetBluetoothDevicesAsDictionaries();

  base::ListValue pairing_method_options;
  pairing_method_options.AppendString(
      bluez::FakeBluetoothDeviceClient::kPairingMethodNone);
  pairing_method_options.AppendString(
      bluez::FakeBluetoothDeviceClient::kPairingMethodPinCode);
  pairing_method_options.AppendString(
      bluez::FakeBluetoothDeviceClient::kPairingMethodPassKey);

  base::ListValue pairing_action_options;
  pairing_action_options.AppendString(
      bluez::FakeBluetoothDeviceClient::kPairingActionDisplay);
  pairing_action_options.AppendString(
      bluez::FakeBluetoothDeviceClient::kPairingActionRequest);
  pairing_action_options.AppendString(
      bluez::FakeBluetoothDeviceClient::kPairingActionConfirmation);
  pairing_action_options.AppendString(
      bluez::FakeBluetoothDeviceClient::kPairingActionFail);

  // Send the list of devices to the view.
  web_ui()->CallJavascriptFunctionUnsafe(
      kUpdateBluetoothInfoJSCallback, *predefined_devices, devices,
      pairing_method_options, pairing_action_options);
}

void DeviceEmulatorMessageHandler::HandleRequestBluetoothPair(
    const base::ListValue* args) {
  // Create the device if it does not already exist.
  std::string path = CreateBluetoothDeviceFromListValue(args);
  bluez::FakeBluetoothDeviceClient::Properties* props =
      fake_bluetooth_device_client_->GetProperties(dbus::ObjectPath(path));

  // Try to pair the device with the main adapter. The device is identified
  // by its device ID, which, in this case is the same as its address.
  ash::WmShell::Get()->system_tray_delegate()->ConnectToBluetoothDevice(
      props->address.value());
  if (!props->paired.value()) {
    web_ui()->CallJavascriptFunctionUnsafe(kPairFailedJSCallback,
                                           base::StringValue(path));
  }
}

void DeviceEmulatorMessageHandler::HandleRequestAudioNodes(
    const base::ListValue* args) {
  // Get every active audio node and create a dictionary to
  // send it to JavaScript.
  base::ListValue audio_nodes;
  for (const AudioNode& node : fake_cras_audio_client_->node_list()) {
    std::unique_ptr<base::DictionaryValue> audio_node(
        new base::DictionaryValue());

    audio_node->SetBoolean("isInput", node.is_input);
    audio_node->SetString("id", base::Uint64ToString(node.id));
    audio_node->SetString("deviceName", node.device_name);
    audio_node->SetString("type", node.type);
    audio_node->SetString("name", node.name);
    audio_node->SetBoolean("active", node.active);

    audio_nodes.Append(std::move(audio_node));
  }
  web_ui()->CallJavascriptFunctionUnsafe(kUpdateAudioNodes, audio_nodes);
}

void DeviceEmulatorMessageHandler::HandleInsertAudioNode(
    const base::ListValue* args) {
  AudioNode audio_node;
  const base::DictionaryValue* device_dict = nullptr;

  CHECK(args->GetDictionary(0, &device_dict));
  CHECK(device_dict->GetBoolean("isInput", &audio_node.is_input));
  CHECK(device_dict->GetString("deviceName", &audio_node.device_name));
  CHECK(device_dict->GetString("type", &audio_node.type));
  CHECK(device_dict->GetString("name", &audio_node.name));
  CHECK(device_dict->GetBoolean("active", &audio_node.active));

  std::string tmp_id;
  CHECK(device_dict->GetString("id", &tmp_id));
  CHECK(base::StringToUint64(tmp_id, &audio_node.id));

  fake_cras_audio_client_->InsertAudioNodeToList(audio_node);
}

void DeviceEmulatorMessageHandler::HandleRemoveAudioNode(
    const base::ListValue* args) {
  std::string tmp_id;
  uint64_t id;
  CHECK(args->GetString(0, &tmp_id));
  CHECK(base::StringToUint64(tmp_id, &id));

  fake_cras_audio_client_->RemoveAudioNodeFromList(id);
}

void DeviceEmulatorMessageHandler::HandleSetHasTouchpad(
    const base::ListValue* args) {
  bool has_touchpad;
  CHECK(args->GetBoolean(0, &has_touchpad));

  system::InputDeviceSettings::Get()->GetFakeInterface()->set_touchpad_exists(
      has_touchpad);
}

void DeviceEmulatorMessageHandler::HandleSetHasMouse(
    const base::ListValue* args) {
  bool has_mouse;
  CHECK(args->GetBoolean(0, &has_mouse));

  system::InputDeviceSettings::Get()->GetFakeInterface()->set_mouse_exists(
      has_mouse);
}

void DeviceEmulatorMessageHandler::UpdateTimeToEmpty(
    const base::ListValue* args) {
}

void DeviceEmulatorMessageHandler::UpdateTimeToFull(
    const base::ListValue* args) {
}

void DeviceEmulatorMessageHandler::UpdatePowerSources(
    const base::ListValue* args) {
}

void DeviceEmulatorMessageHandler::UpdatePowerSourceId(
    const base::ListValue* args) {
}

void DeviceEmulatorMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      kInitialize,
      base::Bind(&DeviceEmulatorMessageHandler::Init, base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kRequestPowerInfo,
      base::Bind(&DeviceEmulatorMessageHandler::RequestPowerInfo,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kUpdateTimeToEmpty,
      base::Bind(&DeviceEmulatorMessageHandler::UpdateTimeToEmpty,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kUpdateTimeToFull,
      base::Bind(&DeviceEmulatorMessageHandler::UpdateTimeToFull,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kUpdatePowerSources,
      base::Bind(&DeviceEmulatorMessageHandler::UpdatePowerSources,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kUpdatePowerSourceId,
      base::Bind(&DeviceEmulatorMessageHandler::UpdatePowerSourceId,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kRequestAudioNodes,
      base::Bind(&DeviceEmulatorMessageHandler::HandleRequestAudioNodes,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kInsertAudioNode,
      base::Bind(&DeviceEmulatorMessageHandler::HandleInsertAudioNode,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kRemoveAudioNode,
      base::Bind(&DeviceEmulatorMessageHandler::HandleRemoveAudioNode,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kBluetoothDiscoverFunction,
      base::Bind(&DeviceEmulatorMessageHandler::HandleRequestBluetoothDiscover,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kBluetoothPairFunction,
      base::Bind(&DeviceEmulatorMessageHandler::HandleRequestBluetoothPair,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kRequestBluetoothInfo,
      base::Bind(&DeviceEmulatorMessageHandler::HandleRequestBluetoothInfo,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kRemoveBluetoothDevice,
      base::Bind(&DeviceEmulatorMessageHandler::HandleRemoveBluetoothDevice,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kSetHasTouchpad,
      base::Bind(&DeviceEmulatorMessageHandler::HandleSetHasTouchpad,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kSetHasMouse,
      base::Bind(&DeviceEmulatorMessageHandler::HandleSetHasMouse,
                 base::Unretained(this)));
}

void DeviceEmulatorMessageHandler::OnJavascriptAllowed() {
  bluetooth_observer_.reset(new BluetoothObserver(this));
  cras_audio_observer_.reset(new CrasAudioObserver(this));

  system::InputDeviceSettings::Get()->TouchpadExists(
      base::Bind(&DeviceEmulatorMessageHandler::TouchpadExists,
          weak_ptr_factory_.GetWeakPtr()));
  system::InputDeviceSettings::Get()->MouseExists(
      base::Bind(&DeviceEmulatorMessageHandler::MouseExists,
          weak_ptr_factory_.GetWeakPtr()));
}

void DeviceEmulatorMessageHandler::OnJavascriptDisallowed() {
  bluetooth_observer_.reset();
  cras_audio_observer_.reset();
}

std::string DeviceEmulatorMessageHandler::CreateBluetoothDeviceFromListValue(
    const base::ListValue* args) {
  const base::DictionaryValue* device_dict = nullptr;
  bluez::FakeBluetoothDeviceClient::IncomingDeviceProperties props;

  CHECK(args->GetDictionary(0, &device_dict));
  CHECK(device_dict->GetString("path", &props.device_path));
  CHECK(device_dict->GetString("name", &props.device_name));
  CHECK(device_dict->GetString("alias", &props.device_alias));
  CHECK(device_dict->GetString("address", &props.device_address));
  CHECK(device_dict->GetString("pairingMethod", &props.pairing_method));
  CHECK(device_dict->GetString("pairingAuthToken", &props.pairing_auth_token));
  CHECK(device_dict->GetString("pairingAction", &props.pairing_action));
  CHECK(device_dict->GetInteger("classValue", &props.device_class));
  CHECK(device_dict->GetBoolean("isTrusted", &props.is_trusted));
  CHECK(device_dict->GetBoolean("incoming", &props.incoming));

  // Create the device and store it in the FakeBluetoothDeviceClient's observed
  // list of devices.
  fake_bluetooth_device_client_->CreateDeviceWithProperties(
      dbus::ObjectPath(bluez::FakeBluetoothAdapterClient::kAdapterPath), props);

  return props.device_path;
}

std::unique_ptr<base::DictionaryValue>
DeviceEmulatorMessageHandler::GetDeviceInfo(
    const dbus::ObjectPath& object_path) {
  // Get the device's properties.
  bluez::FakeBluetoothDeviceClient::Properties* props =
      fake_bluetooth_device_client_->GetProperties(object_path);
  std::unique_ptr<base::DictionaryValue> device(new base::DictionaryValue());
  std::unique_ptr<base::ListValue> uuids(new base::ListValue);
  bluez::FakeBluetoothDeviceClient::SimulatedPairingOptions* options =
      fake_bluetooth_device_client_->GetPairingOptions(object_path);

  device->SetString("path", object_path.value());
  device->SetString("name", props->name.value());
  device->SetString("alias", props->alias.value());
  device->SetString("address", props->address.value());
  if (options) {
    device->SetString("pairingMethod", options->pairing_method);
    device->SetString("pairingAuthToken", options->pairing_auth_token);
    device->SetString("pairingAction", options->pairing_action);
  } else {
    device->SetString("pairingMethod", "");
    device->SetString("pairingAuthToken", "");
    device->SetString("pairingAction", "");
  }
  device->SetInteger("classValue", props->bluetooth_class.value());
  device->SetBoolean("isTrusted", props->trusted.value());
  device->SetBoolean("incoming", false);

  for (const std::string& uuid : props->uuids.value()) {
    uuids->AppendString(uuid);
  }

  device->Set("uuids", std::move(uuids));

  return device;
}

void DeviceEmulatorMessageHandler::TouchpadExists(bool exists) {
  if (!IsJavascriptAllowed())
    return;
  web_ui()->CallJavascriptFunctionUnsafe(kTouchpadExistsCallback,
      base::FundamentalValue(exists));
}

void DeviceEmulatorMessageHandler::MouseExists(bool exists) {
  if (!IsJavascriptAllowed())
    return;
  web_ui()->CallJavascriptFunctionUnsafe(kMouseExistsCallback,
      base::FundamentalValue(exists));
}

}  // namespace chromeos
