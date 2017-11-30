#ifndef ITKPARTICLEGENERALSHAPEMATRIX_H
#define ITKPARTICLEGENERALSHAPEMATRIX_H

#include "itkParticleAttribute.h"
#include "itkDataObject.h"
#include "itkWeakPointer.h"
#include "itkParticleContainer.h"
#include "vnl/vnl_matrix.h"

#include "itkParticleImplicitSurfaceDomain.h"
#include "itkParticleImageDomainWithGradients.h"
#include "TriMesh.h"

namespace itk
{
/** \class ParticleGeneralShapeMatrix
 *
 * \brief Each column describes a shape.  A shape may be composed of
 * m_DomainsPerShape domains (default 1). ALL DOMAINS ARE *NOT* ASSUMED TO HAVE THE
 * SAME NUMBER OF PARTICLES!
 *
 *
 * Each column represents a single shape.
 */
template <class T, unsigned int VDimension>
class ITK_EXPORT ParticleGeneralShapeMatrix
        : public vnl_matrix<T>, public ParticleAttribute<VDimension>
{
public:
    /** Standard class typedefs */
    typedef T DataType;
    typedef ParticleGeneralShapeMatrix Self;
    typedef ParticleAttribute<VDimension> Superclass;
    typedef SmartPointer<Self>  Pointer;
    typedef SmartPointer<const Self>  ConstPointer;
    typedef WeakPointer<const Self>  ConstWeakPointer;

    /** Method for creation through the object factory. */
    itkNewMacro(Self)

    /** Run-time type information (and related methods). */
    itkTypeMacro(ParticleGeneralShapeMatrix, ParticleAttribute)

    virtual void BeforeIteration() {}
    virtual void AfterIteration() {}

    /** Set/Get the number of domains per shape.  This can only be safely done
      before shapes are initialized with points! */
    void SetDomainsPerShape(int i)
    { m_DomainsPerShape = i;     }
    int GetDomainsPerShape() const
    { return m_DomainsPerShape;  }

    void SetAttributesPerDomain(const std::vector<int> &i)
    { m_AttributesPerDomain = i; }

    void SetAttributeScales( const std::vector<double> &s)
    { m_AttributeScales = s;     }

    void SetXYZ(int i, bool val)
    {
        if (m_use_xyz.size() != m_DomainsPerShape)
            m_use_xyz.resize(m_DomainsPerShape);
        m_use_xyz[i] = val;
    }
    void SetNormals(int i, bool val)
    {
        if (m_use_normals.size() != m_DomainsPerShape)
            m_use_normals.resize(m_DomainsPerShape);
        m_use_normals[i] = val;
    }

    virtual void SetMatrix(const vnl_matrix<T> &m)
    {
        vnl_matrix<T>::operator=(m);
    }

    virtual void ResizeMatrix(int rs, int cs)
    {
        vnl_matrix<T> tmp(*this); // copy existing  matrix

        // Create new column (shape)
        this->set_size(rs, cs);

        // Copy old data into new matrix.
        for (unsigned int c = 0; c < tmp.cols(); c++)
        {
            for (unsigned int r = 0; r < tmp.rows(); r++)
                this->operator()(r,c) = tmp(r,c);
        }
    }

    virtual void DomainAddEventCallback(Object *, const EventObject &e)
    {
        const itk::ParticleDomainAddEvent &event = dynamic_cast<const itk::ParticleDomainAddEvent &>(e);
        unsigned int d = event.GetDomainIndex();

        if ( d % m_DomainsPerShape  == 0 )
            this->ResizeMatrix(this->rows(), this->cols()+1);
    }

    virtual void PositionAddEventCallback(Object *o, const EventObject &e)
    {
        // update the size of matrix based on xyz, normals and number of attributes being used
        const itk::ParticlePositionAddEvent &event = dynamic_cast<const itk::ParticlePositionAddEvent &>(e);
        const itk::ParticleSystem<VDimension> *ps= dynamic_cast<const itk::ParticleSystem<VDimension> *>(o);
        const int d = event.GetDomainIndex();
        const unsigned int idx = event.GetPositionIndex();
        const typename itk::ParticleSystem<VDimension>::PointType pos = ps->GetTransformedPosition(idx, d);

        int numRows = 0;
        for (int i = 0; i < m_DomainsPerShape; i++)
        {
            if (m_use_xyz[i])
                numRows += VDimension * ps->GetNumberOfParticles(i);
            if (m_use_normals[i])
                numRows += VDimension * ps->GetNumberOfParticles(i);
             numRows += m_AttributesPerDomain[i];
        }

        if (numRows > this->rows())
            this->ResizeMatrix(numRows, this->cols());

        //setting values now
        unsigned int k = 0;
        int dom = d % m_DomainsPerShape;
        int num = 0;
        for (int i = 0; i < dom; i++)
        {
            if (m_use_xyz[i])
            {
                k += VDimension * ps->GetNumberOfParticles(i);
                num += VDimension;
            }
            if (m_use_normals[i])
            {
                k += VDimension * ps->GetNumberOfParticles(i);
                num += VDimension;
            }
            k += m_AttributesPerDomain[i];
            num += m_AttributesPerDomain[i];
        }
        if (m_use_xyz[dom])
            k += idx * VDimension;
        if (m_use_normals[dom])
            k += idx * VDimension;
        k += idx * m_AttributesPerDomain[dom];

        const ParticleImplicitSurfaceDomain<float, 3>* domain
                = static_cast<const ParticleImplicitSurfaceDomain<float ,3>*>(ps->GetDomain(d));

        const ParticleImageDomainWithGradients<float, 3> * domainWithGrad
             = static_cast<const ParticleImageDomainWithGradients<float ,3> *>(ps->GetDomain(d));

        TriMesh *ptr = domain->GetMesh();
        int s = 0;
        if (m_use_xyz[dom])
        {
            for (unsigned int i = 0; i < VDimension; i++)
                this->operator()(i+k, d / m_DomainsPerShape) = pos[i]*m_AttributeScales[num+i];
            k += VDimension;
            s += 3;
        }
        if (m_use_normals[dom])
        {
            ParticleImageDomainWithGradients::VnlVectorType pN = domainWithGrad->SampleNormalVnl(pos);
            for (unsigned int i = 0; i < VDimension; i++)
                this->operator()(i+k, d / m_DomainsPerShape) = pN[i]*m_AttributeScales[num+i+s];
            k += VDimension;
            s += 3;
        }

        std::vector<float> fVals;
        if (m_AttributesPerDomain[dom] > 0)
        {
            point pt;
            pt.clear();
            pt[0] = pos[0];
            pt[1] = pos[1];
            pt[2] = pos[2];
            fVals.clear();
            ptr->GetFeatureValues(pt, fVals);
        }

        for (int aa = 0; aa < m_AttributesPerDomain[dom]; aa++)
            this->operator()(aa+k, d / m_DomainsPerShape) = fVals[aa]*m_AttributeScales[aa+num+s];

    }

    virtual void PositionSetEventCallback(Object *o, const EventObject &e)
    {
        // update xyz, normals and number of attributes being used
        const itk::ParticlePositionSetEvent &event = dynamic_cast<const itk::ParticlePositionSetEvent &>(e);
        const itk::ParticleSystem<VDimension> *ps= dynamic_cast<const itk::ParticleSystem<VDimension> *>(o);
        const int d = event.GetDomainIndex();
        const unsigned int idx = event.GetPositionIndex();
        const typename itk::ParticleSystem<VDimension>::PointType pos = ps->GetTransformedPosition(idx, d);

        unsigned int k = 0;
        int dom = d % m_DomainsPerShape;
        int num = 0;
        for (int i = 0; i < dom; i++)
        {
            if (m_use_xyz[i])
            {
                k += VDimension * ps->GetNumberOfParticles(i);
                num += VDimension;
            }
            if (m_use_normals[i])
            {
                k += VDimension * ps->GetNumberOfParticles(i);
                num += VDimension;
            }
            k += m_AttributesPerDomain[i];
            num += m_AttributesPerDomain[i];
        }
        if (m_use_xyz[dom])
            k += idx * VDimension;

        if (m_use_normals[dom])
            k += idx * VDimension;
        k += idx * m_AttributesPerDomain[dom];

        const ParticleImplicitSurfaceDomain<float, 3>* domain
                = static_cast<const ParticleImplicitSurfaceDomain<float ,3>*>(ps->GetDomain(d));

        const ParticleImageDomainWithGradients<float, 3> * domainWithGrad
             = static_cast<const ParticleImageDomainWithGradients<float ,3> *>(ps->GetDomain(d));

        TriMesh *ptr = domain->GetMesh();

        int s = 0;
        if (m_use_xyz[dom])
        {
            for (unsigned int i = 0; i < VDimension; i++)
                this->operator()(i+k, d / m_DomainsPerShape) = pos[i]*m_AttributeScales[num+i];
            k += VDimension;
            s += VDimension;
        }
        if (m_use_normals[dom])
        {
            ParticleImageDomainWithGradients::VnlVectorType pN = domainWithGrad->SampleNormalVnl(pos);
            for (unsigned int i = 0; i < VDimension; i++)
                this->operator()(i+k, d / m_DomainsPerShape) = pN[i]*m_AttributeScales[num+i+s];
            k += VDimension;
            s += VDimension;
        }

        std::vector<float> fVals;
        if (m_AttributesPerDomain[dom] > 0)
        {
            point pt;
            pt.clear();
            pt[0] = pos[0];
            pt[1] = pos[1];
            pt[2] = pos[2];
            fVals.clear();
            ptr->GetFeatureValues(pt, fVals);
        }

        for (int aa = 0; aa < m_AttributesPerDomain[dom]; aa++)
            this->operator()(aa+k, d / m_DomainsPerShape) = fVals[aa]*m_AttributeScales[aa+num+s];
    }

    virtual void PositionRemoveEventCallback(Object *, const EventObject &)
    {
        // NEED TO IMPLEMENT THIS
    }

protected:
        ParticleGeneralShapeMatrix()
    {
        m_DomainsPerShape = 1;
        m_use_xyz = true;
        m_use_normals = false;

        this->m_DefinedCallbacks.DomainAddEvent = true;
        this->m_DefinedCallbacks.PositionAddEvent = true;
        this->m_DefinedCallbacks.PositionSetEvent = true;
        this->m_DefinedCallbacks.PositionRemoveEvent = true;
    }
    virtual ~ParticleGeneralShapeMatrix() {}

    void PrintSelf(std::ostream& os, Indent indent) const
    {  Superclass::PrintSelf(os,indent);  }

    int m_DomainsPerShape;
private:

    ParticleGeneralShapeMatrix(const Self&); //purposely not implemented
    void operator=(const Self&); //purposely not implemented

    std::vector<bool> m_use_xyz;
    std::vector<bool> m_use_normals;
    std::vector<unsigned int> m_AttributesPerDomain;
    std::vector<unsigned int> m_AttributeScales;

}; // end class

} // end namespace

#endif // ITKPARTICLEGENERALSHAPEMATRIX_H
