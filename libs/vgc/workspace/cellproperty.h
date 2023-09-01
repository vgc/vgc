//
//A0, A1 -> A2
//A0, A1 -> A0
//
//struct A : KeyEdgeData {
//
//    struct DelayMergeEntry {
//        Color c;
//        float area;
//    };
//
//    void assignMerge(array<cellprop*> tree) override {
//        delayedMergeEntries.append(other.delayedMergeEntry());
//    }
//
//    DelayMergeEntry delayedMergeEntry() {
//        return { color, face->area() };
//    }
//
//    core::Array<DelayMergeEntry> delayedMergeEntries;
//};
//
//plugindata0
//plugindata1
//
//cellproperty virtual class
//{
//   int id()
//};
//
//registry :id -> 
//array<cellproperty*> cellproperties // ex: {style*, plugindata0*, plugindata1*}
//
//map<node, tree<cellproperties>> trees[]
//
//for node in nodes:
//    # tree is map
//    nodeTree = trees[node]
//    node->finalize(nodeTree)
//
//
//glue -> interpolate
//uncut -> merge
//
//

//
//A::merge(dA, B, dB) {
//    this->merge_(this, dA, B, dB) || B->merge_(this, dA, B, dB);
//}
//