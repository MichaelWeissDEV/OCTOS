#ifndef IEXECUTIONBACKEND_H
#define IEXECUTIONBACKEND_H

#include <functional>
#include <memory>
#include "../core/CompilationRequest.h"
#include "../core/CompilationResult.h"

class IExecutionBackend {
public:
    virtual ~IExecutionBackend() = default;

    virtual CompilationResult execute(const CompilationRequest& req,
                                      const std::function<bool()>& isCancelled = nullptr) = 0;

    virtual void cancel(quint64 jobId) = 0;
};

#endif // IEXECUTIONBACKEND_H
