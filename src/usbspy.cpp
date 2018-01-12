#include "usbspy.h"

using namespace Nan; // NOLINT(build/namespaces)

std::mutex m;
std::condition_variable cv;
bool ready = false;

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

		if (data != NULL) {
			v8::Local<v8::Array> device_list;

			if (data->drive_letter == "All") {
				std::list<Device *> dvlst = GetUSBDevices();
				std::list<Device*>::iterator it;

				v8::Local<v8::Array> device_list = New<v8::Array>(dvlst.size());
				
				int i = 0;
				for (it = dvlst.begin(); it != dvlst.end(); ++it)
				{
					v8::Local<v8::Object> obj = Nan::New<v8::Object>();

					Nan::Set(
					obj,
					Nan::New("device_number").ToLocalChecked(),
					New<v8::Number>(it->device_number));

					Nan::Set(
					obj,
					Nan::New("device_status").ToLocalChecked(),
					New<v8::Number>(it->device_status));
					Nan::Set(
					obj,
					Nan::New("vendor_id").ToLocalChecked(),
					New<v8::String>(it->vendor_id.c_str()).ToLocalChecked());
					Nan::Set(
					obj,
					Nan::New("serial_number").ToLocalChecked(),
					New<v8::String>(it->serial_number.c_str()).ToLocalChecked());
					Nan::Set(
					obj,
					Nan::New("product_id").ToLocalChecked(),
					New<v8::String>(it->product_id.c_str()).ToLocalChecked());
					Nan::Set(
					obj,
					Nan::New("drive_letter").ToLocalChecked(),
					New<v8::String>(it->drive_letter.c_str()).ToLocalChecked());

					Nan::Set(device_list, i, obj);
					i++;
				}
			}
			else {

				v8::Local<v8::Array> device_list = New<v8::Array>(1);
				v8::Local<v8::Object> obj = Nan::New<v8::Object>();

				Nan::Set(
					obj,
					Nan::New("device_number").ToLocalChecked(),
					New<v8::Number>(it->device_number));
				Nan::Set(
					obj,
					Nan::New("device_status").ToLocalChecked(),
					New<v8::Number>(it->device_status));
				Nan::Set(
					obj,
					Nan::New("vendor_id").ToLocalChecked(),
					New<v8::String>(it->vendor_id.c_str()).ToLocalChecked());
				Nan::Set(
					obj,
					Nan::New("serial_number").ToLocalChecked(),
					New<v8::String>(it->serial_number.c_str()).ToLocalChecked());
				Nan::Set(
					obj,
					Nan::New("product_id").ToLocalChecked(),
					New<v8::String>(it->product_id.c_str()).ToLocalChecked());
				Nan::Set(
					obj,
					Nan::New("drive_letter").ToLocalChecked(),
					New<v8::String>(it->drive_letter.c_str()).ToLocalChecked());

				Nan::Set(device_list, 0, obj);
			}

			/*Nan::Set(
				obj,
				Nan::New("device_number").ToLocalChecked(),
				New<v8::Number>(data->device_number));
			Nan::Set(
				obj,
				Nan::New("device_status").ToLocalChecked(),
				New<v8::Number>(data->device_status));
			Nan::Set(
				obj,
				Nan::New("vendor_id").ToLocalChecked(),
				New<v8::String>(data->vendor_id.c_str()).ToLocalChecked());
			Nan::Set(
				obj,
				Nan::New("serial_number").ToLocalChecked(),
				New<v8::String>(data->serial_number.c_str()).ToLocalChecked());
			Nan::Set(
				obj,
				Nan::New("product_id").ToLocalChecked(),
				New<v8::String>(data->product_id.c_str()).ToLocalChecked());
			Nan::Set(
				obj,
				Nan::New("drive_letter").ToLocalChecked(),
				New<v8::String>(data->drive_letter.c_str()).ToLocalChecked());*/

			v8::Local<v8::Value> argv[] = { device_list };

#if !defined(_DEBUG) || defined(_TEST_NODE_) 
			progress->Call(1, argv);
#endif
		}
	}

private:
	Callback * progress;
};

NAN_METHOD(SpyOn)
{
#if defined(_DEBUG) && !defined(_TEST_NODE_) 
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
		std::cout << "Listening..." << std::endl;
	}
	cv.notify_one();

#if defined(_DEBUG) && !defined(_TEST_NODE_) 
	New<v8::FunctionTemplate>(SpyOn)->GetFunction()->CallAsConstructor(0, {});
#endif

}

NODE_MODULE(usbspy, Init)