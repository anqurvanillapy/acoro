#pragma once

#include <list>
#include <csetjmp>
#include <cassert>

#define MAX_ACORO_COUNT 32
#define ACORO_STACKSIZ (1024 * 1024)

namespace acoro {

typedef void (*acoro_func_t)(const void*);

enum {
	ACORO_FUNC_INIT,
	ACORO_FUNC_END,
	ACORO_FUNC_YIELD
};

class asfn;

class stack_pool {
public:
	stack_pool()
	{
		for (int i = 0; i < MAX_ACORO_COUNT; ++i) {
			auto s = new char[ACORO_STACKSIZ];
			q_.push_back(s);
		}
	}

	~stack_pool()
	{
		for (auto s : q_) delete[] s;
	}

	stack_pool(const stack_pool&)               = delete;
	stack_pool& operator=(const stack_pool&)    = delete;
	stack_pool(stack_pool&&)                    = delete;
	stack_pool& operator=(stack_pool&&)         = delete;

	char*
	acquire()
	{
		if (q_.empty()) return nullptr;
		auto ret = q_.front();
		q_.pop_front();
		return ret;
	}

	void
	release(char* s)
	{
		q_.push_back(s);
	}
private:
	std::list<char*> q_;
};

class sched {
public:
	sched() = default;
	~sched() = default;

	sched(const sched&)             = delete;
	sched& operator=(const sched&)  = delete;
	sched(sched&&)                  = delete;
	sched& operator=(sched&&)       = delete;

	bool full() const;
	void add(acoro_func_t f, const void* arg);
	void run();
	void yield();

	static asfn* current_asfn_;
	static asfn* asfn_list_;

	static stack_pool pool_;
	static int nasfn_;

	static std::jmp_buf main_env_;
};

asfn*           sched::current_asfn_{};
asfn*           sched::asfn_list_   {};
stack_pool      sched::pool_        {};
int             sched::nasfn_       {};
std::jmp_buf    sched::main_env_    {};

class asfn {
public:
	asfn(int id, acoro_func_t f, const void* arg, char* sp)
		: id_{id}
		, func_{f}
		, arg_{arg}
		, hp_{(unsigned long)sp}
		, sp_{hp_ + ACORO_STACKSIZ}
	{
		assert(sp_);
	}

	~asfn() = default;

	asfn(const asfn&)               = delete;
	asfn& operator=(const asfn&)    = delete;
	asfn(asfn&&)                    = delete;
	asfn& operator=(asfn&&)         = delete;

	friend class sched;

	int
	id() const
	{
		return id_;
	}

	void
	init()
	{
		if (::setjmp(env_)) start_();
	}
private:
	static void
	start_()
	{
#if defined(__i386__)
		__asm__ __volatile__(
			"movl %0, %%esp"
			:
			: "r"(sched::current_asfn_->sp_)
			: "%esp"
		);
#elif defined(__x86_64__)
		__asm__ __volatile__(
			"movq %0, %%rsp"
			:
			: "r"(sched::current_asfn_->sp_)
			: "%rsp"
		);
#endif
		sched::current_asfn_->func_(sched::current_asfn_->arg_);
		::longjmp(sched::main_env_, ACORO_FUNC_END);
	}

	asfn* prev{};
	asfn* next{};

	int id_;
	acoro_func_t func_;
	const void* arg_;
	unsigned long hp_;
	unsigned long sp_;
	std::jmp_buf env_{};
};

bool
sched::full() const
{
	return nasfn_ > MAX_ACORO_COUNT;
}

void
sched::add(acoro_func_t f, const void* arg)
{
	assert(0 <= nasfn_ && nasfn_ <= MAX_ACORO_COUNT);
	if (nasfn_ + 1 > MAX_ACORO_COUNT) return;

	auto sp = pool_.acquire();
	auto _asfn = new asfn(nasfn_, f, arg, sp);
	assert(_asfn);
	_asfn->init();
	++nasfn_;

	if (asfn_list_) {
		_asfn->prev = asfn_list_->prev;
		_asfn->next = asfn_list_;
		asfn_list_->prev->next = _asfn;
		asfn_list_->prev = _asfn;
	} else {
		_asfn->prev = _asfn;
		_asfn->next = _asfn;
	}
	asfn_list_ = _asfn;
}

void
sched::run()
{
	if (!asfn_list_) return;

	asfn* _asfn;
	switch (::setjmp(main_env_)) {
	case ACORO_FUNC_INIT:
		current_asfn_ = asfn_list_;
		break;
	case ACORO_FUNC_END:
		_asfn = current_asfn_;
		--nasfn_;
		if (_asfn->next == _asfn) {
			pool_.release((char*)_asfn->hp_);
			asfn_list_ = nullptr;
			delete _asfn;
			return;
		} else {
			current_asfn_ = current_asfn_->next;
			_asfn->prev->next = _asfn->next;
			_asfn->next->prev = _asfn->prev;
			pool_.release((char*)_asfn->hp_);
			delete _asfn;
		}
		break;
	case ACORO_FUNC_YIELD:
		current_asfn_ = current_asfn_->next;
		break;
	}

	assert(current_asfn_);
	::longjmp(current_asfn_->env_, ACORO_FUNC_END);
}

void
sched::yield()
{
	// TODO: `yield' can return value for implementing generators.
	if (!::setjmp(current_asfn_->env_)) ::longjmp(main_env_, ACORO_FUNC_YIELD);
}

} /* namespace acoro */
