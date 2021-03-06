/************************************************************************
 * This file has been generated automatically from                      *
 *                                                                      *
 * src/core/geometry/qgscurvepolygon.h                                  *
 *                                                                      *
 * Do not edit manually ! Edit header and run scripts/sipify.pl again   *
 ************************************************************************/





class QgsCurvePolygon: QgsSurface
{
%Docstring
 Curve polygon geometry type
.. versionadded:: 2.10
%End

%TypeHeaderCode
#include "qgscurvepolygon.h"
%End
  public:
    QgsCurvePolygon();
    QgsCurvePolygon( const QgsCurvePolygon &p );

    bool operator==( const QgsCurvePolygon &other ) const;
    bool operator!=( const QgsCurvePolygon &other ) const;
%Docstring
 :rtype: bool
%End

    ~QgsCurvePolygon();

    virtual QString geometryType() const;

    virtual int dimension() const;

    virtual QgsCurvePolygon *clone() const /Factory/;

    virtual void clear();


    virtual bool fromWkb( QgsConstWkbPtr &wkb );

    virtual bool fromWkt( const QString &wkt );


    virtual QByteArray asWkb() const;

    virtual QString asWkt( int precision = 17 ) const;

    virtual QDomElement asGML2( QDomDocument &doc, int precision = 17, const QString &ns = "gml" ) const;

    virtual QDomElement asGML3( QDomDocument &doc, int precision = 17, const QString &ns = "gml" ) const;

    virtual QString asJSON( int precision = 17 ) const;


    virtual double area() const;

    virtual double perimeter() const;

    virtual QgsPolygonV2 *surfaceToPolygon() const /Factory/;

    virtual QgsAbstractGeometry *boundary() const /Factory/;


    int numInteriorRings() const;
%Docstring
 :rtype: int
%End
    const QgsCurve *exteriorRing() const;
%Docstring
 :rtype: QgsCurve
%End
    const QgsCurve *interiorRing( int i ) const;
%Docstring
 :rtype: QgsCurve
%End

    virtual QgsPolygonV2 *toPolygon( double tolerance = M_PI_2 / 90, SegmentationToleranceType toleranceType = MaximumAngle ) const /Factory/;
%Docstring
 Returns a new polygon geometry corresponding to a segmentized approximation
 of the curve.
 \param tolerance segmentation tolerance
 \param toleranceType maximum segmentation angle or maximum difference between approximation and curve*
 :rtype: QgsPolygonV2
%End

    virtual void setExteriorRing( QgsCurve *ring /Transfer/ );
%Docstring
 Sets the exterior ring of the polygon. The CurvePolygon type will be updated to match the dimensionality
 of the exterior ring. For instance, setting a 2D exterior ring on a 3D CurvePolygon will drop the z dimension
 from the CurvePolygon and all interior rings.
 \param ring new exterior ring. Ownership is transferred to the CurvePolygon.
.. seealso:: setInteriorRings()
.. seealso:: exteriorRing()
%End

    void setInteriorRings( const QList<QgsCurve *> &rings /Transfer/ );
%Docstring
Sets all interior rings (takes ownership)
%End
    virtual void addInteriorRing( QgsCurve *ring /Transfer/ );
%Docstring
Adds an interior ring to the geometry (takes ownership)
%End

    bool removeInteriorRing( int ringIndex );
%Docstring
 Removes an interior ring from the polygon. The first interior ring has index 0.
 The corresponding ring is removed from the polygon and deleted. If a ring was successfully removed
 the function will return true.  It is not possible to remove the exterior ring using this method.
.. seealso:: removeInteriorRings()
 :rtype: bool
%End

    void removeInteriorRings( double minimumAllowedArea = -1 );
%Docstring
 Removes the interior rings from the polygon. If the minimumAllowedArea
 parameter is specified then only rings smaller than this minimum
 area will be removed.
.. versionadded:: 3.0
.. seealso:: removeInteriorRing()
%End

    virtual void draw( QPainter &p ) const;

     virtual void transform( const QgsCoordinateTransform &ct, QgsCoordinateTransform::TransformDirection d = QgsCoordinateTransform::ForwardTransform,
                    bool transformZ = false );
    virtual void transform( const QTransform &t );


    virtual bool insertVertex( QgsVertexId position, const QgsPoint &vertex );

    virtual bool moveVertex( QgsVertexId position, const QgsPoint &newPos );

    virtual bool deleteVertex( QgsVertexId position );


    virtual QgsCoordinateSequence coordinateSequence() const;

    virtual int nCoordinates() const;

    virtual bool isEmpty() const;

    virtual double closestSegment( const QgsPoint &pt, QgsPoint &segmentPt /Out/, QgsVertexId &vertexAfter /Out/, bool *leftOf /Out/ = 0, double epsilon = 4 * DBL_EPSILON ) const;


    virtual bool nextVertex( QgsVertexId &id, QgsPoint &vertex /Out/ ) const;


    virtual bool hasCurvedSegments() const;


    virtual QgsAbstractGeometry *segmentize( double tolerance = M_PI_2 / 90, SegmentationToleranceType toleranceType = MaximumAngle ) const /Factory/;

%Docstring
 Returns a geometry without curves. Caller takes ownership
 \param tolerance segmentation tolerance
 \param toleranceType maximum segmentation angle or maximum difference between approximation and curve*
 :rtype: QgsAbstractGeometry
%End

    virtual double vertexAngle( QgsVertexId vertex ) const;

%Docstring
 Returns approximate rotation angle for a vertex. Usually average angle between adjacent segments.
  \param vertex the vertex id
  :return: rotation in radians, clockwise from north
 :rtype: float
%End

    virtual int vertexCount( int part = 0, int ring = 0 ) const;

    virtual int ringCount( int part = 0 ) const;

    virtual int partCount() const;

    virtual QgsPoint vertexAt( QgsVertexId id ) const;


    virtual bool addZValue( double zValue = 0 );

    virtual bool addMValue( double mValue = 0 );

    virtual bool dropZValue();

    virtual bool dropMValue();


    virtual QgsCurvePolygon *toCurveType() const /Factory/;

  protected:


    virtual QgsRectangle calculateBoundingBox() const;

};

/************************************************************************
 * This file has been generated automatically from                      *
 *                                                                      *
 * src/core/geometry/qgscurvepolygon.h                                  *
 *                                                                      *
 * Do not edit manually ! Edit header and run scripts/sipify.pl again   *
 ************************************************************************/
