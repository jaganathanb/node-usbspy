#include "usbspy.h"

using namespace Nan; // NOLINT(build/namespaces)

std::mutex m;
std::condition_variable cv;
bool ready = false;

//#define __TEST_MODE__ 1

void processData(const typename AsyncProgressQueueWorker<Device>::ExecutionProgress& progress);

template<typename T>
class ProgressQueueWorker : public AsyncProgressQueueWorker<T> {
public:
	ProgressQueueWorker(Callback *callback, Callback *progress) : AsyncProgressQueueWorker<T>(callback), progress(progress) {

	}

	~ProgressQueueWorker() {
		delete progress;
	}

	void Execute(const typename AsyncProgressQueueWorker<T>::ExecutionProgress& progress) {
		std::unique_lock<std::mutex> lk(m);

		processData(progress);

		while (ready) {
			cv.wait(lk);
		}

		lk.unlock();
		cv.notify_one();
	}

	void HandleProgressCallback(const T *data, size_t count) {
		HandleScope scope;
		v8::Local<v8::Object> obj = Nan::New<v8::Object>();


		Nan::Set(
			obj,
			Nan::New("deviceNumber").ToLocalChecked(),
			New<v8::Number>(data->deviceNumber));
		Nan::Set(
			obj,
			Nan::New("deviceStatus").ToLocalChecked(),
			New<v8::Number>(data->deviceStatus));
		Nan::Set(
			obj,
			Nan::New("vendorId").ToLocalChecked(),
			New<v8::String>(data->vendorId.c_str()).ToLocalChecked());
		Nan::Set(
			obj,
			Nan::New("serialNumber").ToLocalChecked(),
			New<v8::String>(data->serialNumber.c_str()).ToLocalChecked());
		Nan::Set(
			obj,
			Nan::New("productId").ToLocalChecked(),
			New<v8::String>(data->productId.c_str()).ToLocalChecked());
		Nan::Set(
			obj,
			Nan::New("driveLetter").ToLocalChecked(),
			New<v8::String>(data->driveLetter.c_str()).ToLocalChecked());

		v8::Local<v8::Value> argv[] = { obj };

#ifndef __TEST_MODE__
		progress->Call(1, argv);
#endif // __TEST_MODE__

	}

private:
	Callback * progress;
};

NAN_METHOD(SpyOn)
{
#ifdef __TEST_MODE__
	Callback *progress = new Callback();
	Callback *callback = new Callback();
#else 
	Callback *progress = new Callback(To<v8::Function>(info[0]).ToLocalChecked());
	Callback *callback = new Callback(To<v8::Function>(info[1]).ToLocalChecked());
#endif // __TEST_MODE__

	AsyncQueueWorker(new ProgressQueueWorker<Device>(callback, progress));
}

NAN_METHOD(SpyOff)
{
	{
		std::lock_guard<std::mutex> lk(m);
		ready = false;
		std::cout << "main() signals data  not ready for processing\n" << std::endl;
	}
	cv.notify_one();
}

NAN_MODULE_INIT(Init)
{
	Set(target, New<v8::String>("spyOn").ToLocalChecked(), New<v8::FunctionTemplate>(SpyOn)->GetFunction());
	Set(target, New<v8::String>("spyOff").ToLocalChecked(), New<v8::FunctionTemplate>(SpyOff)->GetFunction());
	StartSpying();
}

void StartSpying()
{
	{
		std::lock_guard<std::mutex> lk(m);
		ready = true;
		std::cout << "main() signals data ready for processing\n" << std::endl;
	}
	cv.notify_one();

#ifdef __TEST_MODE__
	New<v8::FunctionTemplate>(SpyOn)->GetFunction()->CallAsConstructor(0, {});
#endif // !__TEST_MODE__

}

NODE_MODULE(usbspy, Init)