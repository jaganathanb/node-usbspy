#include "usbspy.h"

using namespace Nan;

std::mutex m;
std::condition_variable cv;
bool ready = false;

void processData(const typename AsyncProgressQueueWorker<Device>::ExecutionProgress &progress);
v8::Local<v8::Value> Preparev8Object(const Device *data);

template <typename T>
class ProgressQueueWorker : public AsyncProgressQueueWorker<T>
{
  public:
	ProgressQueueWorker(Callback *callback, Callback *progress) : AsyncProgressQueueWorker<T>(callback), progress(progress)
	{
	}

	~ProgressQueueWorker()
	{
		delete progress;
	}

	void Execute(const typename AsyncProgressQueueWorker<T>::ExecutionProgress &progress)
	{
		std::unique_lock<std::mutex> lk(m);

		processData(progress);

		while (ready)
		{
			cv.wait(lk);
		}

		lk.unlock();
		cv.notify_one();
		ClearUSBDeviceList();
	}

	void HandleProgressCallback(const T *data, size_t count)
	{
		HandleScope scope;

		if (data)
		{
			v8::Local<v8::Value> obj = Preparev8Object(data);
			v8::Local<v8::Value> argv[] = {obj};

#if !defined(_DEBUG) || defined(_TEST_NODE_)
			progress->Call(1, argv);
#endif
		}
	}

  private:
	Callback *progress;
};

NAN_METHOD(SpyOn)
{
#if defined(l_DEBUG) && !defined(l_TEST_NODE_)
	Callback *progress = new Callback();
	Callback *callback = new Callback();
#else
	Callback *progress = new Callback(To<v8::Function>(info[0]).ToLocalChecked());
	Callback *callback = new Callback(To<v8::Function>(info[1]).ToLocalChecked());
#endif

	AsyncQueueWorker(new ProgressQueueWorker<Device>(callback, progress));
}

NAN_METHOD(SpyOff)
{
	{
		std::lock_guard<std::mutex> lk(m);
		ready = false;
	}
	cv.notify_one();
}

NAN_METHOD(GetAvailableUSBDevices)
{
	std::list<Device *> usbs = GetUSBDevices();
	v8::Local<v8::Array> result = Nan::New<v8::Array>(usbs.size());

	std::list<Device *>::iterator it;
	int i = 0;
	for (it = usbs.begin(); it != usbs.end(); ++it)
	{
		v8::Local<v8::Value> val = Preparev8Object(*it);
		Nan::Set(result, i, val);
		i++;
	}

	info.GetReturnValue().Set(result);
}

NAN_METHOD(GetUSBDeviceByPropertyName)
{
	std::string param1(*v8::String::Utf8Value(info[0]->ToString()));

	Device *device = GetUSBDeviceByPropertyName(param1);

	info.GetReturnValue().Set(Preparev8Object(device));
}

NAN_MODULE_INIT(Init)
{
	Set(target, New<v8::String>("spyOn").ToLocalChecked(), New<v8::FunctionTemplate>(SpyOn)->GetFunction());
	Set(target, New<v8::String>("spyOff").ToLocalChecked(), New<v8::FunctionTemplate>(SpyOff)->GetFunction());
	Set(target, New<v8::String>("getAvailableUSBDevices").ToLocalChecked(), New<v8::FunctionTemplate>(GetAvailableUSBDevices)->GetFunction());
	Set(target, New<v8::String>("getUSBDeviceByPropertyName").ToLocalChecked(), New<v8::FunctionTemplate>(GetUSBDeviceByPropertyName)->GetFunction());
	StartSpying();
}

v8::Local<v8::Value> Preparev8Object(const Device *data)
{
	v8::Local<v8::Object> device = Nan::New<v8::Object>();

	Nan::Set(
		device,
		Nan::New("device_number").ToLocalChecked(),
		New<v8::Number>(data->device_number));
	Nan::Set(
		device,
		Nan::New("device_status").ToLocalChecked(),
		New<v8::Number>(data->device_status));
	Nan::Set(
		device,
		Nan::New("vendor_id").ToLocalChecked(),
		New<v8::String>(data->vendor_id.c_str()).ToLocalChecked());
	Nan::Set(
		device,
		Nan::New("serial_number").ToLocalChecked(),
		New<v8::String>(data->serial_number.c_str()).ToLocalChecked());
	Nan::Set(
		device,
		Nan::New("product_id").ToLocalChecked(),
		New<v8::String>(data->product_id.c_str()).ToLocalChecked());
	Nan::Set(
		device,
		Nan::New("drive_letter").ToLocalChecked(),
		New<v8::String>(data->drive_letter.c_str()).ToLocalChecked());

	return device;
}

void StartSpying()
{
	{
		std::lock_guard<std::mutex> lk(m);
		ready = true;
		std::cout << "Listening..." << std::endl;
	}
	cv.notify_one();

#if defined(_DEBUG) && !defined(_TEST_NODE_)
	New<v8::FunctionTemplate>(SpyOn)->GetFunction()->CallAsConstructor(0, {});
#endif
}

NODE_MODULE(usbspy, Init)