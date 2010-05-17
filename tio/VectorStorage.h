#pragma once


namespace tio {
namespace MemoryStorage
{
	class VectorStorage : 
		boost::noncopyable,
		public ITioStorage
	{
	private:

		vector<ValueAndMetadata> data_;
		string name_, type_;
		EventDispatcher dispatcher_;

		inline ValueAndMetadata& GetInternalRecord(const TioData& key)
		{
			return data_.at(GetRecordNumber(key));
		}

		inline ValueAndMetadata& GetInternalRecord(const TioData* key)
		{
			return GetInternalRecord(*key);
		}

		inline size_t GetRecordNumber(int index)
		{
			//
			// python like index (-1 for last, -2 for before last, so on)
			//
			if(index < 0)
			{
				if(-index > (int)data_.size())
					throw std::invalid_argument("invalid subscript");
				index = data_.size() + index;
			}

			return static_cast<size_t>(index);
		}

		inline size_t GetRecordNumber(const TioData& td)
		{
			return GetRecordNumber(td.AsInt());
		}

		inline size_t GetRecordNumber(const TioData* td)
		{
			return GetRecordNumber(*td);
		}


	public:

		VectorStorage(const string& name, const string& type) :
			name_(name),
			type_(type)
		  {}

		  ~VectorStorage()
		  {
			  return;
		  }

		  virtual string GetName()
		  {
			  return name_;
		  }

		  virtual string GetType()
		  {
			  return type_;
		  }

		  virtual string Command(const string& command)
		  {
			  throw std::invalid_argument("command not supported");
		  }

		  virtual size_t GetRecordCount()
		  {
			  return data_.size();
		  }

		  virtual void PushBack(const TioData& key, const TioData& value, const TioData& metadata)
		  {
			  CheckValue(value);
			  data_.push_back(ValueAndMetadata(value, metadata));

			  dispatcher_.RaiseEvent("push_back", key, value, metadata);
		  }

		  virtual void PushFront(const TioData& key, const TioData& value, const TioData& metadata)
		  {
			  CheckValue(value);
			  data_.insert(data_.begin(), ValueAndMetadata(value, metadata));

			  dispatcher_.RaiseEvent("push_front", key, value, metadata);
		  }

	private:
		void _Pop(vector<ValueAndMetadata>::iterator i, TioData* value, TioData* metadata)
		{
			ValueAndMetadata& data = *i;

			if(value)
				*value = data.value;

			if(metadata)
				*metadata = data.metadata;

			data_.erase(i);
		}
	public:

		virtual void PopBack(TioData* key, TioData* value, TioData* metadata)
		{
			if(data_.empty())
				throw std::invalid_argument("empty");

			_Pop(data_.end() - 1, value, metadata);

			dispatcher_.RaiseEvent("pop_back", 
				key ? *key : TIONULL, 
				value ? *value : TIONULL,
				metadata ? *metadata : TIONULL);
		}

		virtual void PopFront(TioData* key, TioData* value, TioData* metadata)
		{
			if(data_.empty())
				throw std::invalid_argument("empty");

			_Pop(data_.begin(), value, metadata);

			dispatcher_.RaiseEvent("pop_front", 
				key ? *key : TIONULL, 
				value ? *value : TIONULL,
				metadata ? *metadata : TIONULL);
		}

		void CheckValue(const TioData& value)
		{
			if(value.Empty())
				throw std::invalid_argument("value??");
		}


		virtual void Set(const TioData& key, const TioData& value, const TioData& metadata)
		{
			CheckValue(value);

			ValueAndMetadata& data = GetInternalRecord(key);

			data = ValueAndMetadata(value, metadata);

			dispatcher_.RaiseEvent("set", key, value, metadata);
		}

		virtual void Insert(const TioData& key, const TioData& value, const TioData& metadata)
		{
			CheckValue(value);

			size_t recordNumber = GetRecordNumber(key);

			// check out of bounds
			GetRecord(key, NULL, NULL, NULL);

			data_.insert(data_.begin() + recordNumber, ValueAndMetadata(value, metadata));

			dispatcher_.RaiseEvent("insert", key, value, metadata);
		}

		virtual void Delete(const TioData& key, const TioData& value, const TioData& metadata)
		{
			size_t recordNumber = GetRecordNumber(key);

			// check out of bounds
			GetRecord(key, NULL, NULL, NULL);

			data_.erase(data_.begin() + recordNumber);

			dispatcher_.RaiseEvent("delete", key, value, metadata);
		}

		virtual void Clear()
		{
			data_.clear();

			dispatcher_.RaiseEvent("clear", TIONULL, TIONULL, TIONULL);
		}

		virtual shared_ptr<ITioResultSet> Query(const TioData& query)
		{
			throw std::runtime_error("not implemented");
		}

		virtual void GetRecord(const TioData& searchKey, TioData* key, TioData* value, TioData* metadata)
		{
			const ValueAndMetadata& data = GetInternalRecord(searchKey);

			if(key)
				*key = searchKey;

			if(value)
				*value = data.value;

			if(metadata)
				*metadata = data.metadata;
		}

		virtual unsigned int Subscribe(EventSink sink, const string& start)
		{
			unsigned int cookie = 0;
			size_t startIndex = 0;
			
			if(!start.empty())
			{
				try
				{
					startIndex = GetRecordNumber(lexical_cast<int>(start));

					//
					// if client asks to start at index 0, it's never
					// an error, even if the vector is empty. So, we'll
					// not test out of bounds if zero
					//
					if(startIndex != 0)
						data_.at(startIndex); 
				}
				catch(std::exception&)
				{
					throw std::invalid_argument("invalid start index");
				}
			}

			cookie = dispatcher_.Subscribe(sink);

			if(start.empty())
				return cookie;
			
			//
			// key is the start index to send
			//
			for(size_t x = startIndex ; x < data_.size() ; x++)
			{
				const ValueAndMetadata& data = data_[x];
				sink("push_back", (int)x, data.value, data.metadata);
			}

			return cookie;
		}
		virtual void Unsubscribe(unsigned int cookie)
		{
			dispatcher_.Unsubscribe(cookie);
		}
	};
}}
