"""STEP/XDE writer: assembly instances are components, part solids are bodies."""
from build123d import Compound
from OCP.STEPConstruct import STEPConstruct
from OCP.Message import Message, Message_Gravity
from OCP.TopLoc import TopLoc_Location
from OCP.BRep import BRep_Builder
from OCP.IFSelect import IFSelect_ReturnStatus
from OCP.Interface import Interface_Static
from OCP.STEPCAFControl import STEPCAFControl_Writer
from OCP.STEPControl import STEPControl_StepModelType
from OCP.TCollection import TCollection_ExtendedString, TCollection_HAsciiString
from OCP.TDataStd import TDataStd_Name
from OCP.TDocStd import TDocStd_Document
from OCP.TopoDS import TopoDS_Compound
from OCP.XCAFDoc import XCAFDoc_DocumentTool, XCAFDoc_ColorType


def export_step(shape, path, **kwargs):
    doc = TDocStd_Document(TCollection_ExtendedString('caddev'))
    XCAFDoc_DocumentTool.SetLengthUnit_s(doc, 0.001)  # millimeters
    shapes = XCAFDoc_DocumentTool.ShapeTool_s(doc.Main())
    colors = XCAFDoc_DocumentTool.ColorTool_s(doc.Main())
    shapes.SetAutoNaming_s(False)

    def metadata(label, name, color):
        TDataStd_Name.Set_s(label, TCollection_ExtendedString(name))
        if color is not None:
            colors.SetColor(label, color.wrapped, XCAFDoc_ColorType.XCAFDoc_ColorGen)
            colors.SetColor(label, color.wrapped, XCAFDoc_ColorType.XCAFDoc_ColorSurf)

    body_names = []

    def add(node):
        if node.children:
            empty = TopoDS_Compound()
            BRep_Builder().MakeCompound(empty)
            label = shapes.AddShape(empty, False)
            for child in node.children:
                definition = add(child)
                occurrence = shapes.AddComponent(label, definition, child.location.wrapped)
                metadata(occurrence, child.label, child.color)
        else:
            local = node.wrapped.Located(TopLoc_Location())
            label = shapes.AddShape(local, False)
            for solid, body in zip(Compound(local).solids(), getattr(node, '_caddev_bodies', ())):
                sublabel = shapes.AddSubShape(label, solid.wrapped)
                body_names.append((solid.wrapped, body.label))
                if sublabel.IsNull():
                    raise RuntimeError(f'Could not label STEP body: {body.label}')
                metadata(sublabel, body.label, body.color)
        metadata(label, node.label, node.color)
        return label

    add(shape)
    shapes.UpdateAssemblies()
    for printer in Message.DefaultMessenger_s().Printers():
        printer.SetTraceLevel(Message_Gravity.Message_Fail)
    writer = STEPCAFControl_Writer()
    writer.SetNameMode(True)
    writer.SetColorMode(True)
    Interface_Static.SetCVal_s('write.step.unit', 'MM')
    if not writer.Transfer(doc, STEPControl_StepModelType.STEPControl_AsIs):
        raise RuntimeError('Could not transfer STEP document')
    # OCCT does not transfer XDE solid subshape names by default. Name the actual
    # representation items, without promoting them to assembly products.
    process = writer.Writer().WS().TransferWriter().FinderProcess()
    for solid, name in body_names:
        item = STEPConstruct.FindEntity_s(process, solid)
        if item is None:
            raise RuntimeError(f'Could not find STEP representation for body: {name}')
        item.SetName(TCollection_HAsciiString(name))
    if writer.Write(str(path)) != IFSelect_ReturnStatus.IFSelect_RetDone:
        raise RuntimeError(f'Could not write STEP file: {path}')
    return True
