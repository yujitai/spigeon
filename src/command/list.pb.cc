// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: src/command/list.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "src/command/list.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace store_list {

namespace {

const ::google::protobuf::Descriptor* list_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  list_reflection_ = NULL;
const ::google::protobuf::Descriptor* list_item_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  list_item_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_src_2fcommand_2flist_2eproto() {
  protobuf_AddDesc_src_2fcommand_2flist_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "src/command/list.proto");
  GOOGLE_CHECK(file != NULL);
  list_descriptor_ = file->message_type(0);
  static const int list_offsets_[1] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(list, itemlist_),
  };
  list_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      list_descriptor_,
      list::default_instance_,
      list_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(list, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(list, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(list));
  list_item_descriptor_ = list_descriptor_->nested_type(0);
  static const int list_item_offsets_[2] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(list_item, member_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(list_item, extra_),
  };
  list_item_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      list_item_descriptor_,
      list_item::default_instance_,
      list_item_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(list_item, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(list_item, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(list_item));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_src_2fcommand_2flist_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    list_descriptor_, &list::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    list_item_descriptor_, &list_item::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_src_2fcommand_2flist_2eproto() {
  delete list::default_instance_;
  delete list_reflection_;
  delete list_item::default_instance_;
  delete list_item_reflection_;
}

void protobuf_AddDesc_src_2fcommand_2flist_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\026src/command/list.proto\022\nstore_list\"V\n\004"
    "list\022\'\n\010itemlist\030\001 \003(\0132\025.store_list.list"
    ".item\032%\n\004item\022\016\n\006member\030\001 \002(\014\022\r\n\005extra\030\002"
    " \001(\014", 124);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "src/command/list.proto", &protobuf_RegisterTypes);
  list::default_instance_ = new list();
  list_item::default_instance_ = new list_item();
  list::default_instance_->InitAsDefaultInstance();
  list_item::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_src_2fcommand_2flist_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_src_2fcommand_2flist_2eproto {
  StaticDescriptorInitializer_src_2fcommand_2flist_2eproto() {
    protobuf_AddDesc_src_2fcommand_2flist_2eproto();
  }
} static_descriptor_initializer_src_2fcommand_2flist_2eproto_;

// ===================================================================

#ifndef _MSC_VER
const int list_item::kMemberFieldNumber;
const int list_item::kExtraFieldNumber;
#endif  // !_MSC_VER

list_item::list_item()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void list_item::InitAsDefaultInstance() {
}

list_item::list_item(const list_item& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void list_item::SharedCtor() {
  _cached_size_ = 0;
  member_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  extra_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

list_item::~list_item() {
  SharedDtor();
}

void list_item::SharedDtor() {
  if (member_ != &::google::protobuf::internal::kEmptyString) {
    delete member_;
  }
  if (extra_ != &::google::protobuf::internal::kEmptyString) {
    delete extra_;
  }
  if (this != default_instance_) {
  }
}

void list_item::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* list_item::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return list_item_descriptor_;
}

const list_item& list_item::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_src_2fcommand_2flist_2eproto();
  return *default_instance_;
}

list_item* list_item::default_instance_ = NULL;

list_item* list_item::New() const {
  return new list_item;
}

void list_item::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (has_member()) {
      if (member_ != &::google::protobuf::internal::kEmptyString) {
        member_->clear();
      }
    }
    if (has_extra()) {
      if (extra_ != &::google::protobuf::internal::kEmptyString) {
        extra_->clear();
      }
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool list_item::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // required bytes member = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadBytes(
                input, this->mutable_member()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(18)) goto parse_extra;
        break;
      }

      // optional bytes extra = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_extra:
          DO_(::google::protobuf::internal::WireFormatLite::ReadBytes(
                input, this->mutable_extra()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectAtEnd()) return true;
        break;
      }

      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void list_item::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // required bytes member = 1;
  if (has_member()) {
    ::google::protobuf::internal::WireFormatLite::WriteBytes(
      1, this->member(), output);
  }

  // optional bytes extra = 2;
  if (has_extra()) {
    ::google::protobuf::internal::WireFormatLite::WriteBytes(
      2, this->extra(), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* list_item::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // required bytes member = 1;
  if (has_member()) {
    target =
      ::google::protobuf::internal::WireFormatLite::WriteBytesToArray(
        1, this->member(), target);
  }

  // optional bytes extra = 2;
  if (has_extra()) {
    target =
      ::google::protobuf::internal::WireFormatLite::WriteBytesToArray(
        2, this->extra(), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int list_item::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // required bytes member = 1;
    if (has_member()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::BytesSize(
          this->member());
    }

    // optional bytes extra = 2;
    if (has_extra()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::BytesSize(
          this->extra());
    }

  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void list_item::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const list_item* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const list_item*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void list_item::MergeFrom(const list_item& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_member()) {
      set_member(from.member());
    }
    if (from.has_extra()) {
      set_extra(from.extra());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void list_item::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void list_item::CopyFrom(const list_item& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool list_item::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000001) != 0x00000001) return false;

  return true;
}

void list_item::Swap(list_item* other) {
  if (other != this) {
    std::swap(member_, other->member_);
    std::swap(extra_, other->extra_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata list_item::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = list_item_descriptor_;
  metadata.reflection = list_item_reflection_;
  return metadata;
}


// -------------------------------------------------------------------

#ifndef _MSC_VER
const int list::kItemlistFieldNumber;
#endif  // !_MSC_VER

list::list()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void list::InitAsDefaultInstance() {
}

list::list(const list& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void list::SharedCtor() {
  _cached_size_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

list::~list() {
  SharedDtor();
}

void list::SharedDtor() {
  if (this != default_instance_) {
  }
}

void list::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* list::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return list_descriptor_;
}

const list& list::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_src_2fcommand_2flist_2eproto();
  return *default_instance_;
}

list* list::default_instance_ = NULL;

list* list::New() const {
  return new list;
}

void list::Clear() {
  itemlist_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool list::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // repeated .store_list.list.item itemlist = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_itemlist:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
                input, add_itemlist()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(10)) goto parse_itemlist;
        if (input->ExpectAtEnd()) return true;
        break;
      }

      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void list::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // repeated .store_list.list.item itemlist = 1;
  for (int i = 0; i < this->itemlist_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      1, this->itemlist(i), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* list::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // repeated .store_list.list.item itemlist = 1;
  for (int i = 0; i < this->itemlist_size(); i++) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        1, this->itemlist(i), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int list::ByteSize() const {
  int total_size = 0;

  // repeated .store_list.list.item itemlist = 1;
  total_size += 1 * this->itemlist_size();
  for (int i = 0; i < this->itemlist_size(); i++) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
        this->itemlist(i));
  }

  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void list::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const list* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const list*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void list::MergeFrom(const list& from) {
  GOOGLE_CHECK_NE(&from, this);
  itemlist_.MergeFrom(from.itemlist_);
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void list::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void list::CopyFrom(const list& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool list::IsInitialized() const {

  for (int i = 0; i < itemlist_size(); i++) {
    if (!this->itemlist(i).IsInitialized()) return false;
  }
  return true;
}

void list::Swap(list* other) {
  if (other != this) {
    itemlist_.Swap(&other->itemlist_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata list::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = list_descriptor_;
  metadata.reflection = list_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace store_list

// @@protoc_insertion_point(global_scope)