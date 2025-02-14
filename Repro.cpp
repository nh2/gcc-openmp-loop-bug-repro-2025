#include <iostream>
#include <iomanip>
#include <array>
#include <omp.h>

#include <gen/Repro.pb.h>

using namespace std;

// The following things make the parallel-loop bug go away (unclear if fixed or just hidden):
// * Enabling GCC UBSan
// * Using Clang

// Inlining this function at the call site (doing the `push_back()` there) fixes the bug.
void pushOneElemTo(vector<uint8_t> & featuresOut) {
  featuresOut.push_back(123);
}

bool reproFun() { // returns true if the bug exists

  // Note: The code
  //     std::array<float, 3> color({ 0, 200000.0002 * i + j, 300000.0003 * i + j });
  // causes warning
  //     Non-constant-expression cannot be narrowed from type 'double' to 'float' in initializer list (fix available)clang(-Wc++11-narrowing)
  // but it does not show that with e.g. OpenCV's `Vec3f`'s normal constuctor (without curly braces),
  // with which I initially found this.

  const size_t N = 2;

  cerr << std::setprecision (17); // to see all floats/doubles in full precision

  cerr << endl << "Serial loop:" << endl;

  vector<vector<std::array<double, 3>>> colors_serial(N);

  for (size_t i = 1; i < N; i++) {
    vector<uint8_t> irrelevantContentsVector;
    pushOneElemTo(irrelevantContentsVector);

    for (size_t j = 0; j < irrelevantContentsVector.size(); j++) {
      std::array<float, 3> color({ 0, 200000.0002 * i + j, 300000.0003 * i + j });
      if (false) { // enabling this print changes the values in `colors_serial`
        cerr << "color = " << color[0] << "," << color[1] << "," << color[2] << endl;
      }

      std::array<double, 3> arr;
      arr[0] = color[2];
      arr[1] = color[1];
      arr[2] = 0;
      colors_serial.at(i).push_back(arr);
      cerr << "i = " << i << " ; color = " << color[0] << "," << color[1] << "," << color[2]
           << " ; arr = " << arr[0] << "," << arr[1] << "," << arr[2] << endl;
    }
  }

  cerr << endl << "Parallel loop:" << endl;

  vector<vector<std::array<double, 3>>> colors_parallel(N);

#pragma omp parallel for schedule(static, 1) // removing this pragma fixes the bug, even though the loop has only 1 iteration; same for changing the loop to run [0,1) instead of [1,2)
  for (size_t i = 1; i < N; i++) {
    vector<uint8_t> irrelevantContentsVector;
    pushOneElemTo(irrelevantContentsVector);

    if (irrelevantContentsVector.size() != 1) {
      cerr << "Unexpected irrelevantContentsVector.size(): " << irrelevantContentsVector.size() << endl;
      exit(1);
    }

    for (size_t j = 0; j < irrelevantContentsVector.size(); j++) {
    // we know that this loop runs only 1 iteration (with `j = 0`) because `irrelevantContentsVector.size()` is 1; yet replacing the loop by just that fixes the bug.
    // size_t j = 0; {
      std::array<float, 3> color({ 0, 200000.0002 * i + j, 300000.0003 * i + j });
      if (true) { // enabling this print fixes the bug
        cerr << "color = " << color[0] << "," << color[1] << "," << color[2] << endl;
      }
      std::array<double, 3> arr;
      arr[0] = color[2];
      arr[1] = color[1];
      arr[2] = 0;
      colors_parallel.at(i).push_back(arr);
      cerr << "i = " << i << " ; color = " << color[0] << "," << color[1] << "," << color[2]
           << " ; arr = " << arr[0] << "," << arr[1] << "," << arr[2] << endl;
    }
  }

  cerr << endl;

  for (size_t i = 0; i < colors_parallel.size(); i++) {
    for (size_t j = 0; j < colors_parallel[i].size(); j++) {
      cerr << "colors_parallel[" << i << "][" << j << "] = " << colors_parallel.at(i).at(j)[0] << "," << colors_parallel.at(i).at(j)[1] << "," << colors_parallel.at(i).at(j)[2] << endl;
      cerr << "colors_serial  [" << i << "][" << j << "] = " << colors_serial  .at(i).at(j)[0] << "," << colors_serial  .at(i).at(j)[1] << "," << colors_serial  .at(i).at(j)[2] << endl;
    }
  }

  cerr << endl;

  cerr << "colors serial==parallel? = " << (colors_serial == colors_parallel) << endl;

  bool hasBug = (colors_serial != colors_parallel);
  return hasBug;
}

void reproFunSerial() {

  const size_t N = 2;

  cerr << std::setprecision (17); // to see all floats/doubles in full precision

  cerr << endl << "Serial loop:" << endl;

  vector<vector<std::array<float, 3>>> colors(N);
  vector<vector<repro::proto::V3>> colorProtoV3s(N);
  for (size_t i = 1; i < N; i++) {
    vector<uint8_t> irrelevantContentsVector;
    pushOneElemTo(irrelevantContentsVector);

    for (size_t j = 0; j < irrelevantContentsVector.size(); j++) {
      std::array<float, 3> color({ 0, 200000.0002 * i + j, 300000.0003 * i + j });
      repro::proto::KeyPoint keypointProto;
      repro::proto::V3* colorProto = keypointProto.mutable_color();
      colorProto->set_x(color[2]);
      colorProto->set_y(color[1]);
      colorProto->set_z(0);
      cerr << "i = " << i << " ; color = " << color[0] << "," << color[1] << "," << color[2]
           << " ; colorProto = " << colorProto->x() << "," << colorProto->y() << "," << colorProto->z() << endl;
      colors.at(i).push_back(color);
      colorProtoV3s.at(i).push_back(*colorProto);
    }
  }

}


void reproFunParallel() {

  const size_t N = 2;

  cerr << std::setprecision (17); // to see all floats/doubles in full precision

  cerr << endl << "Parallel loop:" << endl;

  vector<vector<std::array<float, 3>>> colors(N);
  vector<vector<repro::proto::V3>> colorProtoV3s(N);

#pragma omp parallel for schedule(static, 1) // removing this pragma fixes the bug, even though the loop has only 1 iteration; same for changing the loop to run [0,1) instead of [1,2)
  for (size_t i = 1; i < N; i++) {
    vector<uint8_t> irrelevantContentsVector;
    pushOneElemTo(irrelevantContentsVector);

    for (size_t j = 0; j < irrelevantContentsVector.size(); j++) {
      std::array<float, 3> color({ 0, 200000.0002 * i + j, 300000.0003 * i + j });
      repro::proto::KeyPoint keypointProto;
      repro::proto::V3* colorProto = keypointProto.mutable_color();
      colorProto->set_x(color[2]);
      colorProto->set_y(color[1]);
      colorProto->set_z(0);
      cerr << "i = " << i << " ; color = " << color[0] << "," << color[1] << "," << color[2]
           << " ; colorProto = " << colorProto->x() << "," << colorProto->y() << "," << colorProto->z() << endl;
      colors.at(i).push_back(color);
      colorProtoV3s.at(i).push_back(*colorProto);
    }
  }

}


int main(int argc, const char* argv[]) {
  omp_set_dynamic(0);
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  bool hasBug = reproFun();

  // cerr << endl;
  // cerr << endl;
  // cerr << "separate invocations of equal functions, for assembly diffing:" << endl;

  // cerr << "reproFunSerial:" << endl;
  // reproFunSerial();
  // cerr << "reproFunParallel:" << endl;
  // reproFunParallel();

  cerr << endl;
  cerr << "BUG? " << (hasBug ? "YES" : "no") << endl;

  return EXIT_SUCCESS;
}
