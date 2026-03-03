using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace ContextMenuProfiler.UI.Core.Helpers
{
    public static class ObservableCollectionExtensions
    {
        /// <summary>
        /// Inserts an item into a sorted collection while maintaining the order.
        /// Uses Binary Search for O(log N) efficiency.
        /// </summary>
        public static void InsertSorted<T>(this ObservableCollection<T> collection, T item, Comparison<T> comparer)
        {
            if (collection.Count == 0)
            {
                collection.Add(item);
                return;
            }

            int index = BinarySearch(collection, item, comparer);
            if (index < 0) index = ~index; // Bitwise complement of the first element larger than the item

            collection.Insert(index, item);
        }

        private static int BinarySearch<T>(IList<T> list, T item, Comparison<T> comparer)
        {
            int low = 0;
            int high = list.Count - 1;

            while (low <= high)
            {
                int mid = low + ((high - low) >> 1);
                int order = comparer(list[mid], item);

                if (order == 0) return mid;
                if (order < 0) low = mid + 1;
                else high = mid - 1;
            }

            return ~low;
        }
    }

    /// <summary>
    /// Helper to convert Comparison<T> to IComparer<T> for LINQ OrderBy
    /// </summary>
    public class ComparisonComparer<T> : IComparer<T>
    {
        private readonly Comparison<T> _comparison;
        public ComparisonComparer(Comparison<T> comparison) => _comparison = comparison;
        public int Compare(T? x, T? y) => (x == null || y == null) ? 0 : _comparison(x, y);
    }
}
