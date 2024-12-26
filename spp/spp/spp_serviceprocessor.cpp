#include "serviceprocessor.h"
#include <assert.h>

void spp::ServiceProcessor::Process(IService* host)
{
	mWriteSet.Clear();
	mReadSet.Clear();

	host->Prepare();
	host->Set(mReadSet, mWriteSet);

	int result = sck::Select(mReadSet.GetData(), mWriteSet.GetData(), nullptr, mMicroseconds);
	assert(result > -1);

	host->Process(mReadSet, mWriteSet);
}

void spp::ServiceProcessor::Process(const ulib::List<IService*>& list)
{
	mWriteSet.Clear();
	mReadSet.Clear();

	for (IService* obj : list)
	{
		obj->Prepare();
		obj->Set(mReadSet, mWriteSet);
	}

	int result = sck::Select(mReadSet.GetData(), mWriteSet.GetData(), nullptr, mMicroseconds);
	assert(result > -1);

	for (IService* obj : list)
	{
		obj->Process(mReadSet, mWriteSet);
	}
}