#ifndef ZRT_CHECK_H
#define ZRT_CHECK_H

#define CHECK_EXIT_IF_ZRT_NOT_READY				\
    if ( !is_zrt_ready() ){					\
	ZRT_LOG(L_ERROR, "%s %s", __func__, PROLOG_WARNING);	\
	/*while not initialized completely*/			\
	errno=ENOSYS;						\
	return -1;						\
    }


#endif //ZRT_CHECK
