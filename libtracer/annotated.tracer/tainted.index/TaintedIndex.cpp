#include "TaintedIndex.h"

TaintedIndex::TaintedIndex(const DWORD index)
  : Index(index)
{}

TaintedIndex::TaintedIndex()
  :Index(1)
{}

void TaintedIndex::NextIndex() {
  Index += 1;
}

DWORD TaintedIndex::GetIndex() {
  return Index;
}

void TaintedIndex::PrintIndex(AbstractFormat *aFormat) {
}


