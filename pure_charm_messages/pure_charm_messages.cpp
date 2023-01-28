// This test creates an empty array chare DgElement.
//
// Next it inserts an element on each PE.
//
// Then it performs a number of iterations.
//
// In each iteration, all elements are pinged which triggers a reduction
// checking that the sum of contributed ints is equal to the expected value.
//
// During some iterations (controlled by strides), CkNumPes() new elements are
// inserted into the array chare in one of three ways:
//  - the main chare inserts the new element
//  - a specific array element inserts the new element
//  - a specific member of a group chare inserts the new element
//
// Each array element stores the iteration on which it is created
// The id of a newly created array element is iteration*CkNumPes() + i
// where 0 <= i <= CkNumPes()
//
// During a reduction each element contributes thisIndex*iteration
//
// Current behavior (Oct 20, 2022):
//  - Test fails on latest.  It appears that when an array element or group
//    member not on PE 0 creates a new element, the new element executes the
//    calls to ping starting from iteration 1 and not the current iteration.
//
//  - Test passes on charm v7.0.0
//
//  - Git bisection blames the changes made in PR#3614 (which fixed the
//    load-balancing issue we had encountered earlier this year)
//

#include "pure_charm_messages.hpp"

#include <numeric>
#include <sstream>
#include <string>
#include <vector>

/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ CProxy_DgElement dgElementProxy;

namespace {
static constexpr bool print = true;
static constexpr int number_of_elements = 4;

std::string vec_string(const std::vector<double>& vec) {
  std::stringstream ss{};
  ss << "(";
  if (vec.size() > 0) {
    ss << vec[0];
    if (vec.size() > 1) {
      for (size_t i = 1; i < vec.size(); i++) {
        ss << "," << vec[i];
      }
    }
  }
  ss << ")";
  return ss.str();
}

int node_of(const int element) {
  return element < number_of_elements / 2 ? 0 : 1;
}
}  // namespace

// ============================================================================
//                              NonOwningMessage
// ============================================================================

void* NonOwningMessage::pack(NonOwningMessage* in_message) {
  const int data_size = in_message->data_size;
  const int total_size =
      static_cast<int>(sizeof(NonOwningMessage)) + data_size * sizeof(double);

  CkPrintf("PACK!!\n");

  auto* out_message = reinterpret_cast<NonOwningMessage*>(
      CkAllocBuffer(in_message, total_size));

  memcpy(reinterpret_cast<char*>(out_message), &in_message->data_size,
         sizeof(NonOwningMessage));

  out_message->data =
      reinterpret_cast<double*>(std::addressof(out_message->data)) + 1;

  memcpy(out_message->data, in_message->data, data_size * sizeof(double));

  delete in_message;
  return static_cast<void*>(out_message);
}

NonOwningMessage* NonOwningMessage::unpack(void* in_buffer) {
  auto* out_buffer = reinterpret_cast<NonOwningMessage*>(in_buffer);

  CkPrintf("unPACK!!\n");

  const int data_size = out_buffer->data_size;

  out_buffer->data =
      reinterpret_cast<double*>(std::addressof(out_buffer->data)) + 1;

  return out_buffer;
}

// ============================================================================
//                                   Main
// ============================================================================

Main::Main(CkArgMsg* msg) {
  delete msg;

  if (CkNumNodes() != 2) {
    CkPrintf("Must run on 2 charm-nodes\n");
    CkExit();
  }
  if (CkNumPes() != 2) {
    CkPrintf("Must run on 2 charm-cores\n");
    CkExit();
  }

  if (print) {
    CkPrintf("\n\n");
  }

  dgElementProxy = CProxy_DgElement::ckNew();
  for (int i = 0; i < number_of_elements; i++) {
    int node = node_of(i);
    if (print) {
      CkPrintf("Inserting element %i on node %i\n", i, node);
    }
    dgElementProxy[i].insert(i, node);
  }
  dgElementProxy.doneInserting();

  CkStartQD(CkCallback(CkIndex_Main::start_send_expected_address(), mainProxy));
}

void Main::start_send_expected_address() {
  if (print) {
    CkPrintf("Main is in phase start_send_expected_addresses\n");
  }

  for (int i = 0; i < number_of_elements; i++) {
    dgElementProxy[i].send_expected_address();
  }

  CkStartQD(CkCallback(CkIndex_Main::start_send_messages(), mainProxy));
}

void Main::start_send_messages() {
  if (print) {
    CkPrintf("Main is in phase start_send_messages\n");
  }

  for (int i = 0; i < number_of_elements; i++) {
    dgElementProxy[i].send_message();
  }

  CkStartQD(CkCallback(CkIndex_Main::start_check_messages(), mainProxy));
}

void Main::start_check_messages() {
  if (print) {
    CkPrintf("Main is in phase start_check_messages\n");
  }

  CkStartQD(CkCallback(CkIndex_Main::exit(), mainProxy));
}

void Main::exit() {
  if (print) {
    CkPrintf("Main is in phase exit\n");
  }
  CkExit();
}

// ============================================================================
//                                 DgElement
// ============================================================================

DgElement::DgElement(const int element)
    : my_element(element), my_node(node_of(my_element)) {
  my_data.resize(my_element + 1);
  for (size_t i = 0; i < my_data.size(); i++) {
    my_data[i] = static_cast<double>(i);
  }

  element_to_send_to =
      my_element + 1 == number_of_elements ? 0 : my_element + 1;
  node_of_element_to_send_to = node_of(element_to_send_to);
  element_to_receive_from =
      my_element - 1 == -1 ? number_of_elements - 1 : my_element - 1;
  node_of_element_to_receive_from = node_of(element_to_receive_from);

  CkPrintf("Vector on element %i: %s\n", my_element,
           vec_string(my_data).c_str());
  CkPrintf(
      "Element %d node %d: sending to element %d node %d, receiving from "
      "element %d node %d\n",
      my_element, my_node, element_to_send_to, node_of_element_to_send_to,
      element_to_receive_from, node_of_element_to_receive_from);
}

void DgElement::send_expected_address() {
  std::stringstream ss{};
  ss << my_data.data();

  dgElementProxy[element_to_send_to].receive_expected_address(ss.str());
}

void DgElement::receive_expected_address(const std::string sent_address) {
  address_of_sender_my_data = sent_address;
}

void DgElement::send_message() {
  NonOwningMessage* message = new NonOwningMessage();
  message->data_size = my_data.size();
  message->sent_between_nodes = my_node != node_of_element_to_send_to;
  message->data = my_data.data();

  std::stringstream ss{};
  ss << message;

  if (print) {
    CkPrintf(
        "Element %d, node %d: address of message before send to element %d "
        "node %d = %s\n",
        my_element, my_node, element_to_send_to, node_of_element_to_send_to,
        ss.str().c_str());
  }

  dgElementProxy[element_to_send_to].receive_message(message);
}

void DgElement::receive_message(NonOwningMessage* message) {
  std::stringstream ss{};
  ss << message;
  if (print) {
    CkPrintf(
        "Element %d node %d: address of message after receive from element %d "
        "node %d = %s\n",
        my_element, my_node, element_to_receive_from,
        node_of_element_to_receive_from, ss.str().c_str());
  }

  received_message = message;
  received_data = std::vector<double>(received_message->data_size);
  memcpy(&received_data[0], received_message->data,
         received_data.size() * sizeof(double));

  ss.str("");
  ss << received_message->data;

  if (print) {
    CkPrintf(
        "Element %d: address of received_message.data = %s, "
        "address_of_sender_my_data = %s\n",
        my_element, ss.str().c_str(), address_of_sender_my_data.c_str());
  }
}

#include "pure_charm_messages.def.h"
