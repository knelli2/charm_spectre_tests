#pragma once

#include <vector>

#include "pure_charm_messages.decl.h"

struct NonOwningMessage : public CMessage_NonOwningMessage {
  int data_size;
  bool sent_between_nodes;
  double* data;

  static void* pack(NonOwningMessage* in_message);
  static NonOwningMessage* unpack(void* in_buffer);
};

struct Main : public CBase_Main {
 public:
  Main(CkArgMsg* msg);
  void start_send_expected_address();
  void start_send_messages();
  void start_check_messages();
  void exit();
};

struct DgElement : public CBase_DgElement {
  DgElement(int element);
  void send_expected_address();
  void receive_expected_address(const std::string);
  void send_message();
  void receive_message(NonOwningMessage* message);

  int my_element;
  int my_node;
  int element_to_send_to;
  int node_of_element_to_send_to;
  int element_to_receive_from;
  int node_of_element_to_receive_from;
  std::string address_of_sender_my_data;
  std::vector<double> my_data;
  std::vector<double> received_data;
  NonOwningMessage* received_message;
};
