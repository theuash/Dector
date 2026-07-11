#include "media_decoder.h"
// ponytail: FFmpeg decode implementation in week 2

MediaDecoder::MediaDecoder(QObject* parent) : QObject(parent) {}
MediaDecoder::~MediaDecoder() = default;
bool MediaDecoder::open(const QString&) { return false; }
void MediaDecoder::close() { m_open = false; }
bool MediaDecoder::seek(double) { return false; }
std::shared_ptr<QImage> MediaDecoder::readFrame() { return nullptr; }
