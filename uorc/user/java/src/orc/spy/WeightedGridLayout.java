package orc.spy;

import java.awt.*;
import java.util.*;

/** A layout manager similar to GridLayout, except that the cells are
 * not constrained to be the same size. Instead, cells are initially
 * made as small as possible (based on the components' minimum
 * size). Then, any remaining space is distributed amongst the rows
 * and columns according to user-specified weights. The resulting
 * display forms a grid where each column has the same width and each
 * row has the same height.
 *
 * The number of rows does not need to be explicitly specified: it
 * will be computed automatically based on the number of components in
 * the container and the number of columns.
 *
 * Written by Edwin Olson (ebolson@umich.edu), August 2008.
 **/
public class WeightedGridLayout implements LayoutManager
{
    //////////////////////////////////////////////////
    // Specified by user

    int     hgap, vgap;
    double  columnWeights[];

    //////////////////////////////////////////////////
    // Computed internally
    int     cols;               // invariant: columnweights.length
    int     rows;               // recomputed as #components/cols

    // User can specify row weights individually (stored in hashmap)
    // or specify the default.
    HashMap<Integer, Double> rowWeightsMap = new HashMap<Integer, Double>();
    double defaultRowWeight = 0;

    double rowWeights[] = null; // computed as defaultRowWeight, unless rowWeightMap overrides.

    int minimumWidth = 0;       // minimum dimensions of our whole component.
    int minimumHeight = 0;

    int minimumColumnWidth[];   // minimum size of each row and column.
    int minimumRowHeight[];

    /**
       @param columnWeights Horizontal spacing "weight" per column
       @param hgap Pixel spcacing betweens columns
       @param vgap Pixel spacing between rows
    **/
    public WeightedGridLayout(double columnWeights[],
                              int hgap, int vgap)
    {
        this.hgap = hgap;
        this.vgap = vgap;
        this.columnWeights = normalizeSumToOne(columnWeights);
        this.cols = columnWeights.length;
    }

    public WeightedGridLayout(double columnWeights[])
    {
        this(columnWeights, 4, 4);
    }

    public void setDefaultRowWeight(double v)
    {
        defaultRowWeight = v;
    }

    public void setRowWeight(int row, double v)
    {
        rowWeightsMap.put(row, v);
    }

    public void addLayoutComponent(String name, Component comp)
    {
        // nothing to do. We recompute everything in layoutContainer()
    }

    /** Fetch the component from the parent that will be in column x,
     * row y.
     **/
    Component getComponentXY(Container parent, int x, int y)
    {
        return parent.getComponents()[x + y * cols];
    }

    int sum(int v[])
    {
        int acc = 0;
        for (int i = 0; i < v.length; i++)
            acc += v[i];
        return acc;
    }

    double[] normalizeSumToOne(double v[])
    {
        double acc = 0;
        for (int i = 0; i < v.length; i++)
            acc += v[i];

        // if all weights are zero, uniformly distribute extra space.
        double res[] = new double[v.length];

        for (int i = 0; i < v.length; i++)
            res[i] = (acc == 0) ? 1.0/v.length : v[i] / acc;

        return res;
    }

    /** Compute some basic statistics about the size of the
     * components. This is factored out so that both layoutContainer
     * and getMinimumSize (which need this data) can call it.
     **/
    void computeGeometry(Container parent)
    {
        rows = parent.getComponents().length / cols;

        // build an array of rowWeights
        rowWeights = new double[rows];
        for (int i = 0; i < rowWeights.length; i++) {
            if (rowWeightsMap.get(i)==null)
                rowWeights[i] = defaultRowWeight;
            else
                rowWeights[i] = rowWeightsMap.get(i);
        }
        rowWeights = normalizeSumToOne(rowWeights);

        // Compute sizes of rows/columns
        minimumColumnWidth = new int[cols];
        minimumRowHeight = new int[rows];

        for (int col = 0; col < cols; col++) {
            for (int row = 0; row < rows; row++)  {
                Dimension thisdim = getComponentXY(parent, col, row).getMinimumSize();

                minimumColumnWidth[col] = (int) Math.max(minimumColumnWidth[col],
                                                         thisdim.getWidth());

                minimumRowHeight[row] = (int) Math.max(minimumRowHeight[row],
                                                       thisdim.getHeight());
            }
        }

        minimumWidth = sum(minimumColumnWidth) + hgap * (cols - 1);
        minimumHeight = sum(minimumRowHeight) + vgap * (rows - 1);

    }

    public void layoutContainer(Container parent)
    {
        computeGeometry(parent);

        // how much space is left over, after accounting for the minimum size?
        int extraWidth = parent.getWidth() - minimumWidth;
        int extraHeight = parent.getHeight() - minimumHeight;

        ///////////////////////////////////////////////////////
        // distribute the extra space according to the weights.
        //
        // XXX: if there are many columns/rows, truncation error might
        // result in a noticeable number of unused pixels on the far
        // right and bottom.

        int columnWidth[] = new int[cols];
        for (int col = 0; col < cols; col++)
            columnWidth[col] = (int) (minimumColumnWidth[col] + columnWeights[col]*extraWidth);

        int rowHeight[] = new int[rows];
        for (int row = 0; row < rows; row++)
            rowHeight[row] = (int) (minimumRowHeight[row] + rowWeights[row]*extraHeight);

        ///////////////////////////////////////////////////////
        // Integrate the widths and heights to yield positions.

        int columnPosition[] = new int[cols];
        for (int col = 1; col < cols; col++)
            columnPosition[col] += columnPosition[col-1] + columnWidth[col-1];

        int rowPosition[] = new int[rows];
        for (int row = 1; row < rows; row++)
            rowPosition[row] += rowPosition[row-1] + rowHeight[row-1];

        // Finally, tell all the components where they should be.
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {

                Component c = getComponentXY(parent, col, row);
                c.setSize(new Dimension(columnWidth[col], rowHeight[row]));
                c.setLocation(new Point(columnPosition[col], rowPosition[row]));
            }
        }
    }

    public Dimension minimumLayoutSize(Container parent)
    {
        computeGeometry(parent);
        return new Dimension(minimumWidth, minimumHeight);
    }

    public Dimension preferredLayoutSize(Container parent)
    {
        return minimumLayoutSize(parent);
    }

    public void removeLayoutComponent(Component comp)
    {
        // nothing to do.
    }
}
