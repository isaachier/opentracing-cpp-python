#include <iomanip>
#include <sstream>
#include <pybind11/pybind11.h>
#include <opentracing/span.h>

namespace py = pybind11;

namespace {

template <typename Clock>
typename Clock::time_point timestampFromPyFloat(py::object timestamp)
{
    try {
        const auto timestampDouble = py::cast<double>(timestamp);
        const std::chrono::duration<double> timeSinceEpoch(timestampDouble);
        return typename Clock::time_point() +
               std::chrono::duration_cast<std::chrono::seconds>(
                   timeSinceEpoch);
    } catch (...) {
        return Clock::now();
    }
}

std::string systemTimestamp(const opentracing::SystemClock::time_point& time)
{
    std::ostringstream oss;
    const auto cTime = opentracing::SystemClock::to_time_t(time);
    std::tm tm;
    oss << std::put_time(gmtime_r(&cTime, &tm), "%c %Z");
    return oss.str();
}

}  // anonymous namespace

PYBIND11_MODULE(opentracing_cpp, m) {
    using namespace opentracing;

    py::class_<SpanContext>(m, "SpanContext")
        .def("baggage", [](const SpanContext& sc) {
            std::unordered_map<std::string, std::string> baggage;
            sc.ForeachBaggageItem([&baggage](
                const std::string& key, const std::string& value) {
                baggage[key] = value;
                return true;
            });
            return baggage;
        });

    py::class_<Span>(m, "Span")
        .def("context", &Span::context)
        // TODO: .def("tracer", &Span::tracer)
        .def("set_operation_name", &Span::SetOperationName)
        .def("finish", [](Span& span, py::object finishTime) {
            if (finishTime.is_none()) {
                span.Finish();
            }
            else {
                FinishSpanOptions options;
                options.finish_steady_timestamp =
                    timestampFromPyFloat<SteadyClock>(
                        finishTime);
                span.FinishWithOptions(options);
            }
        }, py::arg("finish_time") = py::none())
        .def("set_tag", &Span::SetTag)
        .def("log_kv", [](Span& span,
                          py::dict keyValues,
                          py::object timestamp) {
            // TODO: Once API allows for logging a dynamic collection, remove
            // this loop and make a single call to `Span::Log`
            // (see https://github.com/opentracing/opentracing-cpp/issues/35).
            for (auto&& pair : keyValues) {
                span.Log({{ std::string(py::str(pair.first)),
                            Value(py::str(pair.second)) }});
            }
            const auto logTime =
                timestampFromPyFloat<SystemClock>(timestamp);
            span.Log({{ "timestamp", systemTimestamp(logTime) }});
        }, py::arg(), py::arg("timestamp") = py::none())
        .def("set_baggage_item", &Span::SetBaggageItem)
        .def("get_baggage_item", &Span::BaggageItem);

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}
