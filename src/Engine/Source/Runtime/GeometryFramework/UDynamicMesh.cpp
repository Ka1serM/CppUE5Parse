#include "Engine/Source/Runtime/GeometryFramework/UDynamicMesh.h"

namespace ue5parse::runtime {

void UDynamicMesh::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    Mesh.Serialize(Ar);
}

}
